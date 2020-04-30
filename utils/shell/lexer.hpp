#pragma once
#include <string>
#include <optional>
#include <queue>

struct Token
{
	enum class Type
	{
		Name,
		Pipe,
		Variable,
		VariableAssignment,
		EndCommand
	};

	std::string data;
	Type type;
};

class Lexer
{
public:
	Lexer(const std::string &source);

	std::optional<Token> peek(int count = 0);
	std::optional<Token> consume(Token::Type type);

private:
	std::optional<Token> parse_name();
	std::optional<Token> parse_single_token(Token::Type type);
	std::optional<Token> parse_variable();
	std::optional<Token> next();

	std::vector<Token> peek_queue;
	const std::string &source;
	int pointer;

};

