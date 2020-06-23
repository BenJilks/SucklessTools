#pragma once
#include "statement.hpp"

namespace DB::Sql
{

    class InsertStatement : public Statement
    {
        friend Parser;

    public:
        virtual SqlResult execute(DataBase&) const override;

    private:
        InsertStatement()
            : Statement(Type::Insert) {}

        std::string m_table;
        std::vector<std::string> m_columns;
        std::vector<std::unique_ptr<ValueNode>> m_values;
    };

}
