#include "lexer.hpp"
#include <iostream>

Lexer::Lexer(const std::string &source)
	: source(source)
	, pointer(0)
{
}

std::optional<Token> Lexer::parse_name()
{
	std::string buffer = "";
	for(;;)
	{
		char c = source[pointer];
		if (std::isspace(c))
			break;

		buffer += c;
		pointer += 1;
	}

	return Token {buffer, Token::Type::Name};
}

std::optional<Token> Lexer::peek(int count)
{
	if (count < peek_queue.size())
		return peek_queue[count];

	std::optional<Token> token;
	for (int i = peek_queue.size() - 1; i < count; i++)
	{
		token = next();

		if (token)
			peek_queue.push_back(*token);
	}

	return token;
}

std::optional<Token> Lexer::consume(Token::Type type)
{
	if (peek_queue.empty())
	{
		auto token = next();
		if (!token)
			return std::nullopt;

		if (token->type == type)
			return token;
		
		peek_queue.push_back(*token);
		return std::nullopt;
	}
	
	auto token = peek_queue[0];
	if (token.type != type)
		return std::nullopt;
	
	peek_queue.erase(peek_queue.begin());
	return token;
}

std::optional<Token> Lexer::next()
{
	while (pointer < source.length())
	{
		char c = source[pointer];

		switch(c)
		{
			case '|': pointer += 1; return Token { std::to_string(c), Token::Type::Pipe };
			default: break;
		}

		if (std::isspace(c))
		{
			pointer += 1;
			continue;
		}

		return parse_name();
	}

	return std::nullopt;
}

