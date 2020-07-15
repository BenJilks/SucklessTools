#pragma once
#include <iostream>
#include <optional>
#include <vector>

class Lexer
{
public:
	enum Type
	{
		None,
		Identifier,
		Number,
		Keyword,
		Symbol,
		StringType,
	};

	struct Token
	{
		std::string data;
		Type type;
	};

	Lexer(std::istream &in) 
		: m_in(in) {}

	void add_keyword(const std::string&);
	void add_symbol(const std::string&);
	void add_string_type(char dilim, bool single_char = false);
	void load(const std::string &path);
	void load(std::istream&&);

	std::optional<Token> peek(int count = 0);
	std::optional<Token> consume(Type type = Type::None, const std::string &data = "");
	std::optional<Token> next();

private:
	enum class State
	{
		Default,
		CheckSymbol,
		Name,
		Number,
		StringType,
		EscapeCode,
	};

	struct StringType
	{
		char dilim;
		bool single_char;
	};

	Type keyword_type(const std::string &buffer) const;

	State m_state { State::Default };
	char m_curr_char { 0 };
	bool m_should_reconsume { false };
	std::vector<Token> m_peek_queue;
	std::istream &m_in;
	
	std::vector<std::string> m_keywords;
	std::vector<std::string> m_symbols;
	std::vector<StringType> m_string_types;

};
