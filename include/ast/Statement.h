#ifndef EBC_STATEMENT_H
#define EBC_STATEMENT_H

#include "Expr.h"
#include <memory>

struct Statement;
typedef std::vector<std::unique_ptr<Statement>> Block;

struct Statement {
	enum Form { DECLARATION, ASSIGNMENT, EXPR, RETURN, IF, WHILE, CONTINUE, BREAK };
	Form form;
	const Token& token;
	Expr expr;
	Statement(const Token& token, Form form): token(token), form(form) { }
	virtual std::vector<Block*> blocks() {
		return std::vector<Block*>();
	}
};

struct Assignment: public Statement {
	Assignment(const Token& token): Statement(token, ASSIGNMENT) { }
	std::vector<std::unique_ptr<AccessTok>> accesses;
};

struct Declaration: public Statement {
	Declaration(const Token& token):
			Statement(token, DECLARATION) { }
	Declaration(const Token& token, const Token& type_token):
			Statement(token, DECLARATION), type_token(&type_token) { }
	const Token* type_token = nullptr;
};

struct If: public Statement {
	If(const Token& token): Statement(token, IF) { }
	Block true_block;
	Block else_block;
	bool true_returns;
	bool else_returns;
	virtual std::vector<Block*> blocks() override {
		return { &true_block, &else_block };
	}
};
struct IfTok: public Tok {
	IfTok(const Token& token): Tok(token, IF) { }
	std::unique_ptr<If> if_statement;
};

struct While: public Statement {
	While(const Token& token): Statement(token, WHILE) { }
	Block block;
	virtual std::vector<Block*> blocks() override {
		return { &block };
	}
};

struct Break: public Statement {
	Break(const Token& token): Statement(token, BREAK) { }
	int amount = 1;
};

#endif //EBC_STATEMENT_H
