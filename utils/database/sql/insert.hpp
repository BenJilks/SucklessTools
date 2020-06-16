#pragma once
#include "statement.hpp"
#include "value.hpp"

namespace DB::Sql
{

    class InsertStatement : public Statement
    {
        friend Parser;

    public:
        virtual SqlResult execute(DataBase&) const override { return {}; }

    private:
        InsertStatement()
            : Statement(Type::Insert) {}

        std::string m_table;
        std::vector<std::string> m_columns;
        std::vector<Value> m_values;
    };

}
