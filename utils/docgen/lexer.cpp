#include "lexer.hpp"
#include <cassert>

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
	m_string_types.push_back({ dilim, single_char });
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
						if (symbol.size() == 1)
							return Token { symbol, Token::Symbol };
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
					// We've got a match
					if (symbol == buffer)
					{
						m_state = State::Default;
						return Token { buffer, Token::Symbol };
					}

					// Check for a parial match
					int index = buffer.size();
					if (index < symbol.size() && symbol[index] == m_curr_char)
						has_partial_match = true;
				}

				// Don't wanna deal with this now
				if (!has_partial_match)
				{
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
					return Token { buffer, Token::Number };					
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
					return Token { dilim + buffer + dilim, Token::StringType };
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

Lexer::Token::Type Lexer::keyword_type(const std::string &buffer) const
{
	for (const auto &keyword : m_keywords)
	{
		if (keyword == buffer)
			return Token::Keyword;
	}

	return Token::Identifier;
}

