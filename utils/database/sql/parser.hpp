#pragma once
#include "lexer.hpp"
#include "statement.hpp"
#include "value.hpp"
#include <functional>

namespace DB::Sql
{

    class Parser
    {
    public:
        static std::shared_ptr<Statement> parse(const std::string &query);

    private:
        Parser(const std::string &query);

        std::shared_ptr<Statement> run();
        std::shared_ptr<Statement> parse_select();
        std::shared_ptr<Statement> parse_insert();
        std::shared_ptr<Statement> parse_create_table();

        std::optional<Value> parse_value();
        void parse_list(std::function<void()>);

        Lexer m_lexer;
    };

}
