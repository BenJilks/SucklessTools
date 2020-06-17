#pragma once
#include "../forward.hpp"
#include "sql.hpp"

namespace DB::Sql
{

    class Statement
    {
    public:
        virtual ~Statement() {}

        enum Type
        {
            Select,
            Insert,
            CreateTable,
        };

        virtual SqlResult execute(DataBase&) const = 0;
        inline Type type() const { return m_type; }

    protected:
        Statement(Type type)
            : m_type(type) {}

    private:
        Type m_type;

    };

}
