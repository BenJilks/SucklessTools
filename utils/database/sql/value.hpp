#pragma once
#include <cassert>

namespace DB::Sql
{

    class Value
    {
    public:
        enum Type
        {
            Integer
        };

        Value(int i)
            : m_type(Integer)
            , m_i(i) {}

        inline Type type() const { return m_type; }
        inline int as_int() const { assert(m_type == Integer); return m_i; }

    private:
        Type m_type;
        union
        {
            int m_i;
        };

    };

}
