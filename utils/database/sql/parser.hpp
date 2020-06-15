#pragma once
#include "lexer.hpp"
#include "statement.hpp"

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

        Lexer m_lexer;
    };

}
