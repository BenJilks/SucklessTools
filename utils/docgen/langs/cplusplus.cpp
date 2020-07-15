#include "cplusplus.hpp"
#include <fstream>
#include <cassert>
using namespace CPP;
using namespace Structure;

CodeUnit Parser::parse(const std::string &path)
{
	std::ifstream stream(path);
	return parse(stream);
}

CodeUnit Parser::parse(std::istream &stream)
{
	Lexer lexer(stream);
	lexer.load("../syntax/cpp.json");

	Parser parser(lexer);
	return parser.run();
}

void Parser::parse_function(Namespace &ns)
{
	auto return_type_name = m_lexer.consume(Lexer::Identifier);
	auto name = m_lexer.consume(Lexer::Identifier);
	assert (type_name);
	assert (name);

	auto func = std::make_unique<Function>(name->data);
	func->set_return_type(DataType(return_type_name->data));

	assert (m_lexer.consume(Lexer::Symbol, "("));
	for (;;)
	{
		auto next = m_lexer.peek();
		assert (next);

		if (next->type == Lexer::Symbol && next->data == ")")
			break;

		auto type_name = m_lexer.consume(Lexer::Identifier);
		auto param_name = m_lexer.consume(Lexer::Identifier);
		assert (type_name);
		assert (param_name);

		func->add_param(param_name->data, DataType(type_name->data));
	}
	assert (m_lexer.consume(Lexer::Symbol, ")"));
	m_functions.push_back(std::move(func));
}

CodeUnit Parser::run()
{
	CodeUnit unit("Test");
	auto &ns = unit.default_namespace();

	for (;;)
	{
		auto token = m_lexer.peek();
		if (!token)
			break;

		switch (token->type)
		{
			case Token::Identifier: parse_function(ns);
			default: assert (false);
		}
	}

	return unit;
}

