#pragma once
#include "statement.hpp"

namespace DB::Sql
{
    
    class UpdateStatement : public Statement
    {
        friend Parser;

    public:
        virtual SqlResult execute(DataBase&) const override;

    private:
        UpdateStatement()
            : Statement(Type::Update) {}
        
        struct Assignment
        {
            std::string column;
            std::unique_ptr<Value> value;
        };
        
        std::string m_table;
        std::vector<Assignment> m_columns;
        std::unique_ptr<Value> m_where;
    };
    
}
