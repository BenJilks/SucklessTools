#include "lexer.hpp"
#include <libjson/libjson.hpp>
#include <cassert>
#include <fstream>

void Lexer::add_keyword(const std::string &keyword)
{
	m_keywords.push_back(keyword);
}

void Lexer::add_symbol(const std::string &symbol)
{
	m_symbols.push_back(symbol);
}

void Lexer::add_string_type(char dilim, bool single_char)
{
	m_string_types.push_back(StringType { dilim, single_char });
}

void Lexer::load(const std::string &path)
{
	std::ifstream stream(path);
	load(std::move(stream));
}

void Lexer::load(std::istream &&stream)
{
	auto config = Json::parse(std::move(stream));
	auto keywords = config["keywords"];
	auto symbols = config["symbols"];
	auto string_types = config["string_types"];

	for (const auto &keyword : keywords.as_array())
		add_keyword(keyword.as_string());

	for (const auto &symbol : symbols.as_array())
		add_symbol(symbol.as_string());

	for (const auto &string_type : string_types.as_array())
		add_string_type(string_type["dilim"].as_string()[0]);
}

std::optional<Lexer::Token> Lexer::peek(int count)
{
	while (count >= m_peek_queue.size())
	{
		auto token = next();
		if (!token)
			return std::nullopt;
		m_peek_queue.push_back(*token);
	}

	return m_peek_queue[count];
}

std::optional<Lexer::Token> Lexer::consume(Type type, const std::string &data)
{
	auto token = peek();
	if (!token)
		return std::nullopt;

	if (type != Type::None && token->type != type)
		return std::nullop;

	if (data != "" && token->data != data)
		return std::nullopt;

	m_peek_queue.pop_front();
	return token;
}

std::optional<Lexer::Token> Lexer::next()
{
	std::string buffer;
	const StringType *curr_string_type = nullptr;
	
	for (;;)
	{
		if (!m_should_reconsume)
			m_curr_char = m_in.get();
		m_should_reconsume = false;
		
		switch (m_state)
		{
			case State::Default:
			{
				if (isspace(m_curr_char))
					break;

				if (!m_curr_char || m_curr_char == -1)
					return std::nullopt;

				// Check symbols
				for (const auto &symbol : m_symbols)
				{
					if (m_curr_char == symbol[0])
					{
						m_state = State::CheckSymbol;
						m_should_reconsume = true;
					}
				}
				if (m_state != State::Default)
					break;
				
				// Check string type
				for (const auto &type : m_string_types)
				{
					if (m_curr_char == type.dilim)
					{
						curr_string_type = &type;
						m_state = State::StringType;
						break;
					}
				}
				if (m_state != State::Default)
					break;

				// Check name
				if (isalpha(m_curr_char))
				{
					m_state = State::Name;
					m_should_reconsume = true;
					break;
				}
				
				// Check number
				if (isdigit(m_curr_char))
				{
					m_state = State::Number;
					m_should_reconsume = true;
					break;
				}

				std::cout << "Unkown char: " << m_curr_char << "\n";
				break;
			}
			
			case State::CheckSymbol:
			{
				buffer += m_curr_char;

				bool has_partial_match = false;
				for (const auto &symbol : m_symbols)
				{
					// Check for a parial match
					int index = buffer.size() - 1;
					if (index < symbol.size() && symbol[index] == m_curr_char)
						has_partial_match = true;
				}

				if (!has_partial_match)
				{
					buffer.pop_back();
					for (const auto &symbol : m_symbols)
					{
						if (symbol == buffer)
						{
							m_state = State::Default;
							m_should_reconsume = true;
							return Token { symbol, Type::Symbol };
						}
					}

					// Don't wanna deal with this now
					m_state = State::Default;
					assert (false);
					break;
				}
				break;
			}
			
			case State::Name:
			{
				if (!isalnum(m_curr_char) && m_curr_char != '_' || !m_curr_char || m_curr_char == -1)
				{
					m_state = State::Default;
					m_should_reconsume = true;
					return Token { buffer, keyword_type(buffer) };
				}
				
				buffer += m_curr_char;
				break;
			}
			
			case State::Number:
			{
				if (!isdigit(m_curr_char) || !m_curr_char || m_curr_char == -1)
				{
					m_state = State::Default;
					m_should_reconsume = true;
					return Token { buffer, Type::Number };					
				}
				
				buffer += m_curr_char;
				break;
			}

			case State::StringType:
			{
				assert (curr_string_type);
				auto dilim = curr_string_type->dilim;

				if (m_curr_char == '\\')
				{
					m_state = State::EscapeCode;
					buffer += m_curr_char;
					break;
				}

				if (m_curr_char == dilim)
				{
					m_state = State::Default;
					return Token { dilim + buffer + dilim, Type::StringType };
				}

				buffer += m_curr_char;
				break;
			}

			case State::EscapeCode:
			{
				buffer += m_curr_char;
				m_state = State::StringType;
				break;
			}
		}
	}
}

Lexer::Type Lexer::keyword_type(const std::string &buffer) const
{
	for (const auto &keyword : m_keywords)
	{
		if (keyword == buffer)
			return Type::Keyword;
	}

	return Type::Identifier;
}

