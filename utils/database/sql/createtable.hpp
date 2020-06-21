#pragma once
#include "statement.hpp"

namespace DB::Sql
{

    class CreateTableStatement : public Statement
    {
        friend Parser;

    public:
        virtual SqlResult execute(DataBase&) const override;

    private:
        CreateTableStatement()
            : Statement(CreateTable) {}

        struct Column
        {
            std::string name;
            std::string type;
            int length;
        };

        std::string m_name;
        std::vector<Column> m_columns;

    };

}
