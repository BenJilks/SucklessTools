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
        Parser(const std::string &query);

        std::shared_ptr<Statement> run();

        inline bool good() const { return m_errors.size() == 0; }
        SqlResult errors_as_result();

    private:

        void expected(const std::string &name);
        void match(Lexer::Type, const std::string &name);
        std::shared_ptr<Statement> parse_select();
        std::shared_ptr<Statement> parse_insert();
        std::shared_ptr<Statement> parse_create_table();
        std::shared_ptr<Statement> parse_update();

        std::unique_ptr<Value> parse_condition(std::unique_ptr<Value> left);
        std::unique_ptr<Value> parse_value();
        void parse_list(std::function<void()>);

        Lexer m_lexer;
        std::vector<std::string> m_errors;
    };

}
