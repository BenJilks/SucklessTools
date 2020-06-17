#include "parser.hpp"
#include "select.hpp"
#include "insert.hpp"
#include "../entry.hpp"
#include <cassert>
#include <iostream>
#include <memory>
using namespace DB::Sql;

Parser::Parser(const std::string &query)
    : m_lexer(query)
{
}

std::shared_ptr<Statement> Parser::parse_select()
{
    assert (m_lexer.consume(Lexer::Select));

    auto select = std::unique_ptr<SelectStatement>(new SelectStatement());
    if (m_lexer.consume(Lexer::Star))
    {
        select->m_all = true;
    }
    else
    {
        for (;;)
        {
            auto token = m_lexer.consume(Lexer::Name);
            assert (token);

            select->m_columns.push_back(token->data);
            if (!m_lexer.consume(Lexer::Comma))
                break;
        }
    }

    assert (m_lexer.consume(Lexer::From));
    auto table = m_lexer.consume(Lexer::Name);
    assert (table);

    select->m_table = table->data;
    return select;
}

std::optional<Value> Parser::parse_value()
{
    auto peek = m_lexer.peek();
    assert (peek);

    switch (peek->type)
    {
        case Lexer::Integer:
            m_lexer.consume();
            return Value(atoi(peek->data.c_str()));
        default:
            break;
    }

    return std::nullopt;
}

std::shared_ptr<Statement> Parser::parse_insert()
{
    assert (m_lexer.consume(Lexer::Insert));
    assert (m_lexer.consume(Lexer::Into));

    auto insert = std::unique_ptr<InsertStatement>(new InsertStatement());
    auto table = m_lexer.consume(Lexer::Name);
    assert (table);

    insert->m_table = table->data;
    assert (m_lexer.consume(Lexer::OpenBrace));
    for (;;)
    {
        if (!m_lexer.peek() || m_lexer.peek()->type == Lexer::CloseBrace)
            break;

        auto column = m_lexer.consume(Lexer::Name);
        assert (column);

        insert->m_columns.push_back(column->data);
        if (!m_lexer.consume(Lexer::Comma))
            break;
    }
    assert (m_lexer.consume(Lexer::CloseBrace));

    assert (m_lexer.consume(Lexer::Values));
    assert (m_lexer.consume(Lexer::OpenBrace));
    for (;;)
    {
        if (!m_lexer.peek() || m_lexer.peek()->type == Lexer::CloseBrace)
            break;

        auto value = parse_value();
        assert (value);

        insert->m_values.push_back(*value);
        if (!m_lexer.consume(Lexer::Comma))
            break;
    }
    assert (m_lexer.consume(Lexer::CloseBrace));

    return std::move(insert);
}

std::shared_ptr<Statement> Parser::run()
{
    auto peek = m_lexer.peek();
    if (!peek)
    {
        // TODO: Error
        assert (false);
    }

    switch (peek->type)
    {
        case Lexer::Select: return parse_select();
        case Lexer::Insert: return parse_insert();
        default:
            // TODO: Error
            assert (false);
    }
}

std::shared_ptr<Statement> Parser::parse(const std::string &query)
{
    Parser parser(query);
    return parser.run();
}
