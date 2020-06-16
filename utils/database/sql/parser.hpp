#pragma once
#include "lexer.hpp"
#include "statement.hpp"
#include "value.hpp"

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
        std::optional<Value> parse_value();

        Lexer m_lexer;
    };

}
