#include "select.hpp"
#include "../database.hpp"
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

        result.m_rows.push_back(Row(m_columns, std::move(*row)));
    }

    return result;
}
