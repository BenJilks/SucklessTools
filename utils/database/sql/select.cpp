#include "select.hpp"
#include "../database.hpp"
#include <cassert>
using namespace DB;
using namespace DB::Sql;

SqlResult SelectStatement::execute(DataBase& db) const
{
    auto table = db.get_table(m_table);
    assert (table);

    SqlResult result;
    for (size_t i = 0; i < table->row_count(); i++)
    {
        auto row = table->get_row(i);
        assert (row);

        if (m_all)
        {
            result.m_rows.push_back(std::move(*row));
            continue;
        }
        result.m_rows.push_back(Row(m_columns, std::move(*row)));
    }

    return result;
}