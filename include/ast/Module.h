#ifndef EBC_ITEM_H
#define EBC_ITEM_H

#include "Statement.h"
#include "Variable.h"

struct Item {
	enum Form { FUNCTION, GLOBAL };
	Item(Form form): form(form) { }
	Form form;
	virtual void _() const { }
};

struct Global: public Item {
	Global(const Token& token, Type type): token(token), var(type), Item(GLOBAL) { }
	const Token& token;
	Variable var;
};

struct Function: public Item {
	Function(const Token& token): token(token), Item(FUNCTION) { }

	inline bool allows(const std::vector<Type*>& arguments) {
		for (size_t i = 0; i < arguments.size(); i++) {
			if (!arguments[i]->has(param_types[i].get())) {
				return false;
			}
		}
		return true;
	}
	inline bool allows(const std::vector<Type>& arguments) {
		for (size_t i = 0; i < arguments.size(); i++) {
			if (!arguments[i].has(param_types[i].get())) {
				return false;
			}
		}
		return true;
	}

	const Token& token;
	Block block;
	Type return_type = Type(Prim::VOID);
	int index = 0;
	std::vector<Type> param_types;
	std::vector<const Token*> param_names;
};

typedef std::vector<std::unique_ptr<Item>> Module;

#endif //EBC_ITEM_H
