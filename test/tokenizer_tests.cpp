#include <Tokenizer.h>
#include "catch.hpp"

TEST_CASE("Tokenize simple", "[tokenizer]") {
	Tokenizer tokenizer("Hello world!");
	auto& tokens = tokenizer.get_tokens();
	REQUIRE(tokens.size() == 3);
	REQUIRE(tokens[0] == Token(Token::Form::IDENT, "Hello"));
	REQUIRE(tokens[1] == Token(Token::Form::IDENT, "world"));
	REQUIRE(tokens[2] == Token(Token::Form::SYMBOL, "!"));
}

TEST_CASE("Tokenize math", "[tokenizer]") {
	Tokenizer tokenizer("x = (2 + y) / 64f32;\nz += y.pow(3);");
	auto& tokens = tokenizer.get_tokens();

	REQUIRE(tokens.size() == 19);
	REQUIRE(tokens[0] == Token(Token::Form::IDENT, "x"));
	REQUIRE(tokens[1] == Token(Token::Form::SYMBOL, "="));

	REQUIRE(tokens[3] == Token(Token::Form::INT, "2"));
	REQUIRE(tokens[3].i() == 2);
	REQUIRE(tokens[8] == Token(Token::Form::FLOAT, "64f32"));
	REQUIRE(tokens[8].f() == 64);

	REQUIRE(tokens[12] == Token(Token::Form::SYMBOL, "+"));
	REQUIRE(tokens[14].ident().size() == 2);
	REQUIRE(tokens[14].ident()[1] == "pow");
}

TEST_CASE("Tokenize with comments", "[tokenizer]") {
	Tokenizer tokenizer("x=4;// see++ \n /**/ return /* RETURNS \n FALSE */ false;");
	auto& tokens = tokenizer.get_tokens();
	REQUIRE(tokens.size() == 8);
	REQUIRE(tokens[1] == Token(Token::Form::SYMBOL, "="));
	REQUIRE(tokens[5] == Token(Token::Form::KW_RETURN, "return"));
	REQUIRE(tokens[6] == Token(Token::Form::KW_FALSE, "false"));
}

TEST_CASE("Tokenize numbers", "[tokenizer]") {
	Tokenizer tokenizer("1 1.1 1i 1f 1f32 .1f64 1.f 1i8 0b11 0x11");
	auto& tokens = tokenizer.get_tokens();
	REQUIRE(tokens[0].suffix == Token::N);
	REQUIRE(tokens[1].suffix == Token::F);
	REQUIRE(tokens[2].suffix == Token::I);
	REQUIRE(tokens[3].suffix == Token::F);
	REQUIRE(tokens[4].suffix == Token::F32);
	REQUIRE(tokens[5].suffix == Token::F64);
	REQUIRE(tokens[6].suffix == Token::F);
	REQUIRE(tokens[7].suffix == Token::I8);
	REQUIRE(tokens[8].form == Token::INT);
	REQUIRE(tokens[8].i() == 3);
	REQUIRE(tokens[9].i() == 17);
}
