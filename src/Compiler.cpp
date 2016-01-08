#include "Compiler.h"
#include "passes/Circuiter.h"
#include "passes/ReturnChecker.h"
#include "passes/TypeChecker.h"
#include "passes/Completer.h"
#include "Filesystem.h"

Compiler::Compiler(const std::string& filename, std::string out_build, std::string out_exec)
		: out_build(out_build), out_exec(out_exec) {
	initialize(filename);

	for (auto& file : files) {
		std::string out_filename;
		for (auto& str : file->module.name) {
			out_filename += str + "-";
		}
		out_filename += ".ll";
		file->out_filename = concat_paths(out_build, out_filename);
	}
	for (auto& file : files) {
		compile(*file);
	}

	std::string command("llvm-link -o \"eb-mass.ll\" -S ");
	for (auto& file : files) {
		command += file->out_filename + " ";
	}
	//std::cout << command << std::endl;
	exec(command.c_str());

	std::string out_s = concat_paths(out_build, "out.s");
	command = "llc -o " + out_s + " \"eb-mass\".ll";
	//std::cout << command << std::endl;
	exec(command.c_str());

	command = "clang -o " + (out_exec.empty() ? "out" : out_exec) + " " + out_s + " shim.a";
	//std::cout << command << std::endl;
	exec(command.c_str());
}

void Compiler::compile(File& file) {
	if (file.state != File::READY) return;
	file.state = File::IN_PROGRESS;

	Parser parser;
	parser.construct(file.module, file.tokens->get_tokens());

	// perform short circuiting transformations
	// (replacing || and && with if's when there are side effects)
	Circuiter circuiter;
	circuiter.shorten(file.module);

	// transforms expression ifs into regular ifs
	// checks every returning function returns on all paths
	// creates implicit returns when possible & necessary
	ReturnChecker return_checker;
	return_checker.check(file.module);

	State state(file.module);

	// Resolve dependencies
	resolve(file.module, state);

	// Infers as much type information as possible.
	// Fails if any types don't match.
	// Also checks validity of breaks/continues for some reason.
	TypeChecker type_checker;
	type_checker.check(file.module, state);

	// Numerically ambiguous variables are set to either I32 or F64.
	// Fails if any types are still unknown.
	// Ambiguous literals are fixed.
	Completer completer;
	completer.complete(file.module, state);

	Builder builder;
	builder.build(file.module, state, file.out_filename);

	file.state = File::FINISHED;
}

void Compiler::initialize(const std::string& filename) {
	std::string name = get_file_stem(filename);
	for (int i = 0; i < name.size(); i++){
		if ((i == 0 && !is_valid_ident_beginning(name[i])) || (i > 0 && !is_valid_ident(name[i]))) {
			throw Exception("'" + filename + "' (" + name + ") is not a proper identifier");
		}
	}

	File* file = new File();
	files.push_back(std::unique_ptr<File>(file));
	file->module.name.push_back(name);
	file->module.add_import(file->module.name, file->module);
	file_tree.add(file->module.name, *file);

	file->stream.open(filename);
	if (!file->stream.is_open()) throw Exception("Could not find '" + filename + "'");
	file->tokens.reset(new Tokenizer(file->stream));
	auto& traits = file->tokens->get_traits();
	for (auto& trait : traits) {
		switch (trait.first) {
			case Trait::INCLUDE:   initialize(trait.second); break;
			case Trait::OUT_EXEC:  out_exec = trait.second; break;
			case Trait::OUT_BUILD: out_build = trait.second; break;
		}
	}
}

void Compiler::resolve(Module& module, State& state) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = module[i];
		switch (item.form) {
			case Item::IMPORT: {
				Import& import_item = (Import&)item;
				std::vector<std::string> vec;
				for (auto token : import_item.target) {
					vec.push_back(token->str());
				}
				import(module, vec, import_item.token);
			} break;
			case Item::GLOBAL: {
				Global& global = (Global&)item;
				bool exists = module.declare(global);
				if (exists) throw Exception("Global with this name already exists", global.token);
				global.unique_name = combine(module.name, ".") + "." + global.token.str();
			} break;
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				bool exists = module.declare(func);
				if (exists) throw Exception("Duplicate function definition", func.token);
				std::stringstream ss;
				ss << combine(module.name, ".") << "." << func.token.str();
				ss << "." << func.param_names.size() << "." << func.index;
				func.unique_name = ss.str();
				state.descend(func.block);
				state.set_func(func);
				for (size_t j = 0; j < func.param_names.size(); j++) {
					state.declare(func.param_names[j]->str(), func.param_types[j]).is_param = true;
				}
				resolve(module, func.block, state);
				state.ascend();
			} break;
		}
	}
}

void Compiler::resolve(Module& module, const Block& block, State& state) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		switch (statement.form) {
			case Statement::DECLARATION: {
				Declaration& decl = (Declaration&)statement;
				if (decl.type_token == nullptr) {
					state.declare(statement.token.str(), Type());
				} else {
					state.declare(statement.token.str(), Type::parse(decl.type_token->str()));
				}
			} break;
			default: break;
		}
		for (Block* inner_block : statement.blocks()) {
			state.descend(*inner_block);
			resolve(module, *inner_block, state);
			state.ascend();
		}
		resolve(module, statement.expr, state);
	}
}
void Compiler::resolve(Module& module, const Expr& expr, State& state) {
	for (size_t i = 0; i < expr.size(); i++) {
		Tok& tok = *expr[i];
		if (tok.token->ident().size() > 1) {
			// if identifier length is 1, it's assumed to be within this module
			auto vec = tok.token->ident();
			vec.pop_back();
			Module* mod = module.search(vec);
			if (mod == nullptr) {
				mod = &import(module, vec, *tok.token);
			}
			if (tok.form == Tok::VAR) {
				Global* global = mod->get_global(tok.token->ident().back());
				if (global == nullptr) throw Exception("Couldn't resolve ident", *tok.token);
				((VarTok&)tok).var = &global->var;
				((VarTok&)tok).external = true;
				module.external_globals.push_back(global);
			} else if (tok.form == Tok::FUNC) {
				FuncTok& ftok = (FuncTok&)tok;
				auto& funcs = mod->get_functions(ftok.num_params, tok.token->ident().back());
				ftok.possible_funcs = funcs;
				ftok.external = true;
			}
		} else if (tok.form == Tok::VAR) {
			Variable* var = state.get_var(tok.token->str());
			if (var == nullptr) throw Exception("Couldn't resolve ident", *tok.token);
			((VarTok&)tok).var = var;
		} else if (tok.form == Tok::FUNC) {
			int num_params = ((FuncTok&)tok).num_params;
			auto& funcs = state.get_module().get_functions(num_params, tok.token->str());
			((FuncTok&)tok).possible_funcs = funcs;
		}
	}
}

Module& Compiler::import(Module& module, const std::vector<std::string>& name, const Token& token) {
	File* file = file_tree.search(name);
	if (file == nullptr) throw Exception("Module could not be found", token);
	if (file->state == File::READY) {
		compile(*file);
	} else if (file->state == File::IN_PROGRESS) {
		throw Exception("Circular dependency!", token);
	}
	module.add_import(name, file->module);
	return file->module;
}
