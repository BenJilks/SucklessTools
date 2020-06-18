#pragma once
#include "statement.hpp"
#include <vector>
#include <string>

namespace DB::Sql
{

    class SelectStatement : public Statement
    {
        friend Parser;

    public:
        virtual SqlResult execute(DataBase&) const override;

    private:
        SelectStatement();

        std::vector<std::string> m_columns;
        std::string m_table;
        std::unique_ptr<Value> m_where;
        bool m_all { false };

    };

}
