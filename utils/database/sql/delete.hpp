#pragma once
#include "statement.hpp"

namespace DB::Sql
{
    
    class DeleteStatement : public Statement
    {
        friend Parser;

    public:
        virtual SqlResult execute(DataBase&) const override;

    private:
        DeleteStatement()
            : Statement(Type::Delete) {}
        
        std::string m_table;
        std::unique_ptr<Value> m_where;
    };
    
}
