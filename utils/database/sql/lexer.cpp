#include "lexer.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
using namespace DB::Sql;

std::optional<Lexer::Token> Lexer::next()
{
    if (m_peek_stack.size() > 0)
    {
        auto token = m_peek_stack.front();
        m_peek_stack.erase(m_peek_stack.begin());
        return token;
    }

    std::string buffer;
    for (;;)
    {
        if (m_pointer > m_query.size())
            break;

        if (!m_should_reconsume)
        {
            m_curr_char = m_query[m_pointer];
            m_pointer += 1;
        }
        m_should_reconsume = false;

        auto c = m_curr_char;
        switch (m_state)
        {
            case State::Normal:
                if (isspace(c))
                    break;

                if (isalpha(c))
                {
                    m_should_reconsume = true;
                    m_state = State::Name;
                    break;
                }

                if (isdigit(c))
                {
                    m_should_reconsume = true;
                    m_state = State::Integer;
                    break;
                }

                switch (c)
                {
                    case '*':
                        return Token { "*", Type::Star };
                    case ',':
                        return Token { ",", Type::Comma };
                    case '(':
                        return Token { "(", Type::OpenBrace };
                    case ')':
                        return Token { ")", Type::CloseBrace };
                    case '>':
                        return Token { ">", Type::MoreThan };
                    default:
                        break;
                }

                assert (false);
                break;

            case State::Name:
                if (!isalnum(c))
                {
                    m_should_reconsume = true;
                    m_state = State::Normal;
                    return parse_name(buffer);
                }

                buffer += c;
                break;

            case State::Integer:
                if (!isdigit(c))
                {
                    m_should_reconsume = true;
                    m_state = State::Normal;
                    return Token { buffer, Type::Integer };
                }

                buffer += c;
                break;
        }
    }

    return std::nullopt;
}

Lexer::Token Lexer::parse_name(const std::string &buffer)
{
    auto lower = buffer;
    std::for_each(lower.begin(), lower.end(), [](char &c)
    {
        c = ::tolower(c);
    });

    if (lower == "select")
        return { buffer, Type::Select };
    else if (lower == "from")
        return { buffer, Type::From };
    else if (lower == "insert")
        return { buffer, Type::Insert };
    else if (lower == "into")
        return { buffer, Type::Into };
    else if (lower == "values")
        return { buffer, Type::Values };
    else if (lower == "create")
        return { buffer, Type::Create };
    else if (lower == "table")
        return { buffer, Type::Table };
    else if (lower == "where")
        return { buffer, Type::Where };
    return { buffer, Type::Name };
}

std::optional<Lexer::Token> Lexer::consume(Type type)
{
    auto token = peek();
    if (!token)
        return std::nullopt;

    if (type != Type::None && token->type != type)
        return std::nullopt;

    return next();
}

std::optional<Lexer::Token> Lexer::peek(size_t count)
{
    std::optional<Token> token;
    for (size_t i = 0; i <= count; i++)
    {
        if (i < m_peek_stack.size())
        {
            token = m_peek_stack[i];
            continue;
        }

        token = next();
        if (!token)
            return std::nullopt;

        m_peek_stack.push_back(*token);
    }

    return token;
}
