#include "passes/Circuiter.h"

void Circuiter::shorten(Module& module) {
	for (size_t i = 0; i < module.size(); i++) {
		Item& item = module[i];
		switch (item.form) {
			case Item::FUNCTION: {
				Function& func = (Function&)item;
				shorten(func.block);
			} break;
			default: break;
		}
	}
}

void Circuiter::shorten(Block& block) {
	for (size_t i = 0; i < block.size(); i++) {
		Statement& statement = *block[i];
		shorten(statement.expr);
	}
}

void Circuiter::shorten(Expr& expr) {
	std::vector<int> has_side_fx_stack;
	std::vector<std::vector<std::unique_ptr<Tok>*>> stack;
	std::vector<std::pair<int, int>> range_stack;
	If* new_if = nullptr;
	const Token* new_if_token = nullptr;
	for (size_t j = 0; j < expr.size(); j++) {
		Tok& tok = *expr[j];
		switch (tok.form) {
			case Tok::INT: case Tok::FLOAT: case Tok::VAR:
				has_side_fx_stack.push_back(false);
				stack.push_back(std::vector<std::unique_ptr<Tok>*>(1, &expr[j]));
				range_stack.push_back(std::make_pair(j, j + 1));
				break;
			case Tok::IF: {
				has_side_fx_stack.push_back(true);
				stack.push_back(std::vector<std::unique_ptr<Tok>*>(1, &expr[j]));
				range_stack.push_back(std::make_pair(j, j + 1));
				shorten(((IfTok&)tok).if_statement->expr);
			} break;
			case Tok::FUNC:
				merge(has_side_fx_stack, stack, range_stack, ((FuncTok&)tok).num_params, true, j);
				stack.back().push_back(&expr[j]);
				break;
			case Tok::OP: {
				Op op = ((OpTok&)tok).op;
				if ((op == Op::AND || op == Op::OR) && has_side_fx_stack.back()) {
					// a || b  -->  if  a { true  } else { b }
					// a && b  -->  if !a { false } else { b }
					new_if = new If(*tok.token);
					auto& cond = stack[stack.size() - 2];
					for (size_t i = 0; i < cond.size(); i++) {
						new_if->expr.emplace_back();
						new_if->expr.back().swap(*cond[i]);
					}
					if (op == Op::AND) new_if->expr.emplace_back(new OpTok(*tok.token, Op::NOT));
					new_if->true_block.emplace_back(new Statement(*tok.token, Statement::EXPR));
					new_if->true_block[0]->expr.emplace_back(new IntTok(*tok.token, op == Op::OR));
					auto& else_expr = stack[stack.size() - 1];
					new_if->else_block.emplace_back(new Statement(*tok.token, Statement::EXPR));
					for (size_t i = 0; i < else_expr.size(); i++) {
						new_if->else_block[0]->expr.emplace_back();
						new_if->else_block[0]->expr.back().swap(*else_expr[i]);
					}
					merge(has_side_fx_stack, stack, range_stack, 2, true, j);
					new_if_token = tok.token;
					goto end_loop;
				} else if (is_binary(op)) {
					merge(has_side_fx_stack, stack, range_stack, 2, false, j);
					stack.back().push_back(&expr[j]);
				} else {
					merge(has_side_fx_stack, stack, range_stack, 1, false, j);
					stack.back().push_back(&expr[j]);
				}
			} break;
		}
	}
	end_loop:
	if (new_if == nullptr) return;
	Expr copy;
	copy.resize(expr.size());
	for (size_t i = 0; i < copy.size(); i++) {
		copy[i].swap(expr[i]);
	}
	expr.clear();
	for (size_t i = 0; i < copy.size(); i++) {
		if (i == range_stack.back().first) {
			IfTok* tok = new IfTok(*new_if_token);
			tok->if_statement.reset(new_if);
			expr.emplace_back(std::unique_ptr<IfTok>(tok));
			i = (size_t)range_stack.back().second - 1;
		} else {
			expr.push_back(std::move(copy[i]));
		}
	}
	shorten(expr);
}

void Circuiter::merge(std::vector<int>& side_fx_stack,
                      std::vector<std::vector<std::unique_ptr<Tok>*>>& stack,
                      std::vector<std::pair<int, int>>& range_stack, int num, bool side_fx,
                      size_t j) {
	std::vector<std::unique_ptr<Tok>*> vec;
	std::pair<int, int> range(j, j + 1);
	for (int i = num; i >= 1; i--) {
		auto& to_add = stack[stack.size() - i];
		vec.insert(vec.end(), to_add.begin(), to_add.end());
		side_fx = side_fx || side_fx_stack[side_fx_stack.size() - i];
		range.first  = std::min(range_stack[range_stack.size() - i].first,  range.first);
		range.second = std::max(range_stack[range_stack.size() - i].second, range.second);
	}
	stack.erase(stack.end() - num, stack.end());
	stack.push_back(vec);
	side_fx_stack.erase(side_fx_stack.end() - num, side_fx_stack.end());
	side_fx_stack.push_back(side_fx);
	range_stack.erase(range_stack.end() - num, range_stack.end());
	range_stack.push_back(range);
}