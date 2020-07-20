#pragma once
#include "statement.hpp"

namespace DB::Sql
{

    class CreateTableStatement : public Statement
    {
        friend Parser;

    public:
        virtual SqlResult execute(DataBase&) const override;

    protected:
        CreateTableStatement(Type type = CreateTable)
            : Statement(type) {}

        inline const std::string &table_name() const { return m_name; }
    private:

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
