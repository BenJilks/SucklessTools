#pragma once
#include "../forward.hpp"
#include "../row.hpp"
#include <string>
#include <vector>

namespace DB
{

    class SqlResult
    {
        friend Sql::Statement;
        friend Sql::SelectStatement;

    public:
        const auto begin() const { return m_rows.begin(); }
        const auto end() const { return m_rows.end(); }

    private:
        SqlResult() {}

        std::vector<Row> m_rows;

    };

}
