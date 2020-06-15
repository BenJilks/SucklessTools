#include "parser.hpp"
#include "select.hpp"
#include "../entry.hpp"
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
    for (;;)
    {
        auto token = m_lexer.consume(Lexer::Name);
        assert (token);

        select->m_columns.push_back(token->data);
        if (!m_lexer.consume(Lexer::Comma))
            break;
    }

    assert (m_lexer.consume(Lexer::From));
    auto table = m_lexer.consume(Lexer::Name);
    assert (table);

    select->m_table = table->data;
    return select;
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
