#include "parser.hpp"
#include "select.hpp"
#include "insert.hpp"
#include "createtable.hpp"
#include "../entry.hpp"
#include <cassert>
#include <iostream>
#include <memory>
using namespace DB;
using namespace DB::Sql;

Parser::Parser(const std::string &query)
    : m_lexer(query)
{
}

SqlResult Parser::errors_as_result()
{
    SqlResult result;
    result.m_errors = m_errors;
    return result;
}

void Parser::expected(const std::string &name)
{
    auto token = m_lexer.consume();
    m_errors.push_back("Expected token '" +
        name + "', got '" +
        token->data + "' instead");
}

void Parser::match(Lexer::Type type, const std::string &name)
{
    if (!m_lexer.consume(type))
        expected(name);
}

void Parser::parse_list(std::function<void()> callback)
{
    match(Lexer::OpenBrace, "(");
    for (;;)
    {
        if (!m_lexer.peek() || m_lexer.peek()->type == Lexer::CloseBrace)
            break;

        callback();

        if (!m_lexer.consume(Lexer::Comma))
            break;
    }
    match(Lexer::CloseBrace, ")");
}

std::unique_ptr<Value> Parser::parse_condition(std::unique_ptr<Value> left)
{
    auto peek = m_lexer.peek();
    if (!peek)
        return left;

    ValueCondition::Operation operation;
    std::unique_ptr<Value> right;
    switch (peek->type)
    {
        case Lexer::MoreThan:
            m_lexer.consume();
            right = parse_value();
            operation = ValueCondition::MoreThan;
        default:
            break;
    }

    if (!right)
        return left;

    return std::make_unique<ValueCondition>(
        std::move(left), operation, std::move(right));
}

std::unique_ptr<Value> Parser::parse_value()
{
    auto peek = m_lexer.peek();
    if (!peek)
        return nullptr;

    std::unique_ptr<Value> value;
    switch (peek->type)
    {
        case Lexer::Integer:
            m_lexer.consume();
            value = std::make_unique<ValueInteger>(atoi(peek->data.c_str()));
        default:
            break;
    }

    if (!value)
        return nullptr;

    return parse_condition(std::move(value));
}

std::shared_ptr<Statement> Parser::parse_select()
{
    match(Lexer::Select, "select");

    auto select = std::shared_ptr<SelectStatement>(new SelectStatement());
    if (m_lexer.consume(Lexer::Star))
    {
        select->m_all = true;
    }
    else
    {
        for (;;)
        {
            auto token = m_lexer.consume(Lexer::Name);
            if (!token)
            {
                expected("column name");
                continue;
            }

            select->m_columns.push_back(token->data);
            if (!m_lexer.consume(Lexer::Comma))
                break;
        }
    }

    match(Lexer::From, "from");
    auto table = m_lexer.consume(Lexer::Name);
    if (!table)
    {
        expected("table name");
        return nullptr;
    }

    if (m_lexer.consume(Lexer::Where))
        select->m_where = parse_value();

    select->m_table = table->data;
    return select;
}

std::shared_ptr<Statement> Parser::parse_insert()
{
    match(Lexer::Insert, "instert");
    match(Lexer::Into, "into");

    auto insert = std::shared_ptr<InsertStatement>(new InsertStatement());
    auto table = m_lexer.consume(Lexer::Name);
    if (!table)
    {
        expected("table name");
        return nullptr;
    }

    insert->m_table = table->data;
    parse_list([&]()
    {
        auto column = m_lexer.consume(Lexer::Name);
        if (!column)
            expected("column name");
        else
            insert->m_columns.push_back(column->data);
    });

    match(Lexer::Values, "values");
    parse_list([&]()
    {
        auto value = parse_value();
        if (!value)
            expected("value");
        else
            insert->m_values.push_back(std::move(value));
    });

    return std::move(insert);
}

std::shared_ptr<Statement> Parser::parse_create_table()
{
    match(Lexer::Create, "create");
    match(Lexer::Table, "table");

    auto create_table = std::shared_ptr<CreateTableStatement>(new CreateTableStatement());
    auto table_name = m_lexer.consume(Lexer::Name);
    if (!table_name)
    {
        expected("table name");
        return nullptr;
    }

    create_table->m_name = table_name->data;
    parse_list([&]()
    {
        auto column_name = m_lexer.consume(Lexer::Name);
        if (!column_name)
        {
            expected("column name");
            return;
        }

        auto column_type = m_lexer.consume(Lexer::Name);
        if (!column_type)
        {
            expected("column type");
            return;
        }

        create_table->m_columns.push_back({column_name->data, column_type->data});
    });

    return std::move(create_table);
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
        case Lexer::Create: return parse_create_table();
        default:
            // TODO: Error
            assert (false);
    }
}
