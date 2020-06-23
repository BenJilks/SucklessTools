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
        std::unique_ptr<ValueNode> m_where;
    };
    
}
