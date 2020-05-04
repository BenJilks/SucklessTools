#include "lexer.hpp"
#include <iostream>

Lexer::Lexer(const std::string &source)
	: source(source)
	, pointer(0)
{
}

static std::string parse_escapes(const std::string &str)
{
	std::string out;

	for (int i = 0; i < str.length(); i++)
	{
		char c = str[i];
		if (c == '\\')
		{
			if (i >= str.length() - 1)
				continue;

			c = str[++i];
			switch (c)
			{
				case '\\': out += '\\'; break;
				case 'n': out += '\n'; break;
				case 'r': out += '\r'; break;
				case 't': out += '\t'; break;
				case '0':
					if (i >= str.length() - 2)
						break;

					if (str[i+1] == '3' && str[i+2] == '3')
					{
						i += 2;
						out += '\033';
					}
					break;
			}

			continue;
		}
		out += c;
	}

	return out;
}

std::string Lexer::parse_string()
{
	std::string buffer;
	pointer += 1; // Skip '"'
	while (pointer < source.length())
	{
		char c = source[pointer];
		if (c == '"')
			break;

		buffer += c;
		pointer += 1;
	}

	pointer += 1; // Skip "
	return parse_escapes(buffer);
}

std::optional<Token> Lexer::parse_name()
{
	std::string buffer;
	auto type = Token::Type::Name;

	for(;;)
	{
		char c = source[pointer];
		if (std::isspace(c) || c == ';' || c == '|')
			break;

		if (c == '=')
			type = Token::Type::VariableAssignment;

		if (c == '"')
		{
			auto str = parse_string();
			buffer += str;
			break;
		}

		buffer += c;
		pointer += 1;

		if (pointer >= source.length())
			break;
	}

	return Token {parse_escapes(buffer), type};
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

std::optional<Token> Lexer::parse_single_token(Token::Type type)
{
	char c = source[pointer];
	pointer += 1;
	
	return Token { std::to_string(c), type };
}

std::optional<Token> Lexer::parse_double_token(
	Token::Type a, Token::Type b, char b_char)
{
	std::string buffer = std::to_string(source[pointer]);
	pointer += 1;

	if (pointer < source.length())
	{
		char other = source[pointer];
		if (other == b_char)
		{
			buffer += other;
			pointer += 1;
			return Token { buffer, b };
		}
	}

	return Token { buffer, a };
}

std::optional<Token> Lexer::parse_variable()
{
	// Skip '$'
	pointer += 1;

	std::string buffer = "";
	for (;;)
	{
		char c = source[pointer];
		if (!std::isalnum(c) && c != '_')
			break;

		buffer += c;
		pointer += 1;
	}

	return Token { buffer, Token::Type::Variable };
}

std::optional<Token> Lexer::next()
{
	while (pointer < source.length())
	{
		char c = source[pointer];

		switch(c)
		{
			case '|': return parse_double_token(Token::Type::Pipe, Token::Type::Or, '|');
			case '&': return parse_double_token(Token::Type::With, Token::Type::And, '&');
			case '\n':
			case ';': return parse_single_token(Token::Type::EndCommand);
			case '$': return parse_variable();
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

