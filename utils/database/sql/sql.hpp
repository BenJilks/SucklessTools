#pragma once
#include "../forward.hpp"
#include "../row.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace DB
{

    class SqlResult
    {
        friend Sql::Parser;
        friend Sql::Statement;
        friend Sql::SelectStatement;
        friend Sql::InsertStatement;
        friend Sql::CreateTableStatement;

    public:
        const auto begin() const { return m_rows.begin(); }
        const auto end() const { return m_rows.end(); }
        
        bool good() { return m_errors.size() == 0; }
        void output_errors(std::ostream &out = std::cerr)
        {
            for (const auto &error : m_errors)
                out << "SQL Error: " << error << "\n";
        }

    private:
        static SqlResult error(std::string message)
        {
            SqlResult result;
            result.m_errors.push_back(message);
            return result;
        }
        static SqlResult ok()
        {
            return {};
        }
        
        SqlResult() {}

        std::vector<Row> m_rows;
        std::vector<std::string> m_errors;

    };

}
