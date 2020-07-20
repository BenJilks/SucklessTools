#include "parser.hpp"
#include "select.hpp"
#include "insert.hpp"
#include "createtable.hpp"
#include "createtableifnotexists.hpp"
#include "update.hpp"
#include "delete.hpp"
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

std::unique_ptr<ValueNode> Parser::parse_condition(std::unique_ptr<ValueNode> left)
{
    auto peek = m_lexer.peek();
    if (!peek)
        return left;

    ValueNode::Type operation;
    std::unique_ptr<ValueNode> right;
    switch (peek->type)
    {
        case Lexer::MoreThan:
            m_lexer.consume();
            right = parse_value();
            operation = ValueNode::Type::MoreThan;
            break;
        case Lexer::Equals:
            m_lexer.consume();
            right = parse_value();
            operation = ValueNode::Type::Equals;
            break;
        default:
            break;
    }

    if (!right)
        return left;

    return std::make_unique<ValueNode>(
        std::move(left), operation, std::move(right));
}

std::unique_ptr<ValueNode> Parser::parse_value()
{
    auto peek = m_lexer.peek();
    if (!peek)
        return nullptr;

    std::unique_ptr<ValueNode> value;
    switch (peek->type)
    {
        case Lexer::Integer:
        {
            m_lexer.consume();
            auto i = atol(peek->data.c_str());
            value = std::make_unique<ValueNode>(Value(i));
            break;
        }
        case Lexer::String:
        {
            m_lexer.consume();
            value = std::make_unique<ValueNode>(Value(peek->data));
            break;
        }
        case Lexer::Name:
        {
            m_lexer.consume();
            auto operand = std::make_unique<ValueNode>(Value(peek->data));
            value = std::make_unique<ValueNode>(ValueNode::Type::Column, std::move(operand));
            break;
        }
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
            if (token)
                select->m_columns.push_back(token->data);
            else
                expected("column name");
            
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
    {
        auto condition = parse_value();
        if (!condition)
        {
            expected("condition");
            return nullptr;
        }

        select->m_where = std::move(condition);
    }

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

    std::shared_ptr<CreateTableStatement> create_table;
    if (m_lexer.consume(Lexer::If))
    {
        match(Lexer::Not, "not");
        match(Lexer::Exists, "exists");

        create_table = std::shared_ptr<CreateTableIfNotExistsStatement>(
            new CreateTableIfNotExistsStatement());
    }
    else
    {
        create_table = std::shared_ptr<CreateTableStatement>(
            new CreateTableStatement());
    }

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

        int column_type_length = 1;
        if (m_lexer.consume(Lexer::OpenBrace))
        {
            auto length = m_lexer.consume(Lexer::Integer);
            if (!length)
            {
                expected("type length");
                return;
            }

            column_type_length = atoi(length->data.c_str());
            match(Lexer::CloseBrace, ")");
        }

        create_table->m_columns.push_back({
            column_name->data, column_type->data, column_type_length});
    });

    return std::move(create_table);
}

std::shared_ptr<Statement> Parser::parse_update()
{
    match(Lexer::Update, "update");

    auto update = std::shared_ptr<UpdateStatement>(new UpdateStatement());
    auto table = m_lexer.consume(Lexer::Name);
    if (!table)
    {
        expected("table name");
        return nullptr;
    }

    update->m_table = table->data;
    match(Lexer::Set, "set");
    for (;;)
    {
        auto column = m_lexer.consume(Lexer::Name);
        if (!column)
        {
            expected("column name");
            return nullptr;
        }

        match(Lexer::Equals, "=");
        auto value = parse_value();
        if (!value)
        {
            expected("value");
            return nullptr;
        }

        update->m_columns.push_back({column->data, std::move(value)});
        if (!m_lexer.consume(Lexer::Comma))
            break;
    }

    if (m_lexer.consume(Lexer::Where))
    {
        auto where = parse_value();
        if (!where)
        {
            expected("value");
            return nullptr;
        }

        update->m_where = std::move(where);
    }

    return update;
}

std::shared_ptr<Statement> Parser::parse_delete()
{
    match(Lexer::Delete, "delete");
    match(Lexer::From, "from");

    auto delete_ = std::shared_ptr<DeleteStatement>(new DeleteStatement());
    auto table = m_lexer.consume(Lexer::Name);
    if (!table)
    {
        expected("table name");
        return nullptr;
    }
    delete_->m_table = table->data;

    match(Lexer::Where, "where");
    auto where = parse_value();
    if (!where)
    {
        expected("condition");
        return nullptr;
    }
    delete_->m_where = std::move(where);

    return delete_;
}

std::shared_ptr<Statement> Parser::run()
{
    auto peek = m_lexer.peek();
    if (!peek)
    {
        m_errors.push_back("No statement given");
        return nullptr;
    }

    switch (peek->type)
    {
        case Lexer::Select: return parse_select();
        case Lexer::Insert: return parse_insert();
        case Lexer::Create: return parse_create_table();
        case Lexer::Update: return parse_update();
        case Lexer::Delete: return parse_delete();
        default:
            m_errors.push_back("Unkown statement '" + peek->data + "'");
            return nullptr;
    }
}
