#include "lexer.hpp"
#include <iostream>
#include <cassert>
#include <iostream>

Lexer::Lexer(const std::string &source)
    : m_source(source)
{
}

std::optional<Token> Lexer::next()
{
    std::string buffer;

    for (;;)
    {
        if (!m_should_reconsume)
            m_curr_char = (m_index < (int)m_source.size()) ? m_source[m_index++] : 0;
        m_should_reconsume = false;

        switch (m_state)
        {
            case State::Default:
                if (!m_curr_char)
                    return std::nullopt;

                if (m_curr_char == '\n')
                    return Token { "\n", Token::Type::EndCommand };

                if (isspace(m_curr_char))
                    break;

                switch (m_curr_char)
                {
                    case '|':
                        m_state = State::Pipe;
                        break;
                    case '&':
                        m_state = State::And;
                        break;
                    case '$':
                        m_state = State::VariableStart;
                        break;
                    case ';':
                        return Token { ";", Token::Type::EndCommand };
                    case '>':
                        m_state = State::Arrow;
                        break;
                    case '2':
                        m_state = State::Two;
                        break;

                    default:
                        m_state = State::Name;
                        m_should_reconsume = true;
                        break;
                }

                break;

            case State::Name:
                if (m_curr_char == '=')
                {
                    buffer += "=";
                    m_state = State::VariableAssignment;
                    break;
                }

                if (m_curr_char == '\\')
                {
                    m_return_state = m_state;
                    m_state = State::Escape;
                    break;
                }

                if (isspace(m_curr_char) || m_curr_char == ';' || m_curr_char == '&' || m_curr_char == '|' || !m_curr_char)
                {
                    m_state = State::Default;
                    m_should_reconsume = true;
                    return Token { buffer, Token::Type::Name };
                }

                buffer += m_curr_char;
                break;

            case State::Pipe:
                if (m_curr_char == '|')
                {
                    m_state = State::Default;
                    return Token { "||", Token::Type::Or };
                }

                m_state = State::Default;
                m_should_reconsume = true;
                return Token { "|", Token::Type::Pipe };

            case State::And:
                if (m_curr_char == '&')
                {
                    m_state = State::Default;
                    return Token { "&&", Token::Type::And };
                }

                if (m_curr_char == '>')
                {
                    m_state = State::AndArrow;
                    break;
                }

                m_state = State::Default;
                m_should_reconsume = true;
                return Token { "&", Token::Type::With };

            case State::AndArrow:
                if (m_curr_char == '>')
                {
                    m_state = State::Default;
                    return Token { "&>>", Token::Type::RedirectAllAppend };
                }

                m_state = State::Default;
                m_should_reconsume = true;
                return Token { "&>", Token::Type::RedirectAll };

            case State::Arrow:
                if (m_curr_char == '>')
                {
                    m_state = State::Default;
                    return Token { ">>", Token::Type::RedirectAppend };
                }

                m_state = State::Default;
                m_should_reconsume = true;
                return Token { ">", Token::Type::Redirect };

            case State::Two:
                if (m_curr_char == '>')
                {
                    m_state = State::TwoArrow;
                    break;
                }

                m_state = State::Name;
                buffer += "2";
                m_should_reconsume = true;
                break;

            case State::TwoArrow:
                if (m_curr_char == '>')
                {
                    m_state = State::Default;
                    return Token { "2>>", Token::Type::RedirectErrAppend };
                }

                m_state = State::Default;
                m_should_reconsume = true;
                return Token { "2>", Token::Type::RedirectErr };

            case State::VariableStart:
                if (m_curr_char == '{')
                {
                    m_state = State::VariableCurly;
                    break;
                }

                if (m_curr_char == '(')
                {
                    m_state = State::SubCommand;
                    break;
                }

                m_state = State::Variable;
                m_should_reconsume = true;
                break;

            case State::Variable:
                if (m_curr_char == '\\')
                {
                    m_return_state = m_state;
                    m_state = State::Escape;
                    break;
                }

                if (!isalnum(m_curr_char))
                {
                    m_state = State::Default;
                    m_should_reconsume = true;
                    return Token { buffer, Token::Type::Variable };
                    break;
                }

                buffer += m_curr_char;
                break;

            case State::VariableCurly:
                if (m_curr_char == '\\')
                {
                    m_return_state = m_state;
                    m_state = State::Escape;
                    break;
                }

                if (m_curr_char == '}' || !m_curr_char)
                {
                    m_state = State::Default;
                    m_should_reconsume = true;
                    return Token { buffer, Token::Type::Variable };
                }

                buffer += m_curr_char;
                break;

            case State::VariableAssignment:
                if (m_curr_char == '"')
                {
                    m_state = State::VariableAssignmentString;
                    break;
                }

                if (m_curr_char == '\\')
                {
                    m_return_state = m_state;
                    m_state = State::Escape;
                    break;
                }

                if (!isalnum(m_curr_char))
                {
                    m_state = State::Default;
                    m_should_reconsume = true;
                    return Token { buffer, Token::Type::VariableAssignment };
                }

                buffer += m_curr_char;
                break;

            case State::VariableAssignmentString:
                if (m_curr_char == '\\')
                {
                    m_return_state = m_state;
                    m_state = State::Escape;
                    break;
                }

                if (m_curr_char == '"' || !m_curr_char)
                {
                    m_state = State::Default;
                    return Token { buffer, Token::Type::VariableAssignment };
                }

                buffer += m_curr_char;
                break;

            case State::SubCommand:
                if (m_curr_char == '\\')
                {
                    m_return_state = m_state;
                    m_state = State::Escape;
                    break;
                }

                if (m_curr_char == ')' || !m_curr_char)
                {
                    m_state = State::Default;
                    m_should_reconsume = true;
                    return Token { buffer, Token::Type::SubCommand };
                }

                buffer += m_curr_char;
                break;

            case State::Escape:
                if (m_curr_char == '0')
                {
                    m_state = State::Escape03;
                    break;
                }

                switch (m_curr_char)
                {
                    case '\\': buffer += '\\'; break;
                    case 'n': buffer += '\n'; break;
                    case 'r': buffer += '\r'; break;
                    case 't': buffer += '\t'; break;
                    default:
                        buffer += "\\";
                        buffer += m_curr_char;
                        break;
                }

                m_state = m_return_state;
                break;

            case State::Escape03:
                if (m_curr_char == '3')
                {
                    m_state = State::Escape033;
                    break;
                }

                buffer += "\\0";
                buffer += m_curr_char;
                m_state = m_return_state;
                break;

            case State::Escape033:
                if (m_curr_char == '3')
                {
                    m_state = m_return_state;
                    buffer += "\033";
                    break;
                }

                buffer += "\\03";
                buffer += m_curr_char;
                m_state = m_return_state;
                break;
        }
    }
}

std::optional<Token> Lexer::peek(int count)
{
    while (count >= (int)m_peek_queue.size())
    {
        auto token = next();
        if (!token)
            return std::nullopt;

        bool should_discard = false;
        if (hook_on_token)
            should_discard = hook_on_token(*token, m_token_index);

        if (!should_discard)
        {
            m_peek_queue.push_back(*token);
            m_token_index += 1;
        }
    }

    return m_peek_queue[count];
}

std::optional<Token> Lexer::consume(Token::Type type)
{
    auto token = peek();
    if (!token)
        return std::nullopt;

    if (token->type != type)
        return std::nullopt;

    m_peek_queue.erase(m_peek_queue.begin());
    return token;
}

void Lexer::insert_string(const std::string &str)
{
    Lexer sub_lexer(str);
    for (;;)
    {
        auto token = sub_lexer.next();
        if (!token)
            break;

        m_peek_queue.push_back(*token);
        m_token_index += 1;
    }
}
