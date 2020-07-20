#include "insert.hpp"
#include "../database.hpp"
#include "value.hpp"
using namespace DB;
using namespace DB::Sql;

SqlResult InsertStatement::execute(DataBase& db) const
{
    if (m_columns.size() != m_values.size())
    {
        assert (false);
        return SqlResult::error("Column and value counts do not match");
    }

    auto table = db.get_table(m_table);
    if (!table)
        return SqlResult::error("No table with the name '" + m_table + "' found");

    auto row = table->make_row();
    for (int i = 0; i < (int)m_columns.size(); i++)
    {
        const auto &column = m_columns[i];
        const auto &value = m_values[i];
        row[column]->set(value->evaluate(row).as_entry());
    }

    table->add_row(std::move(row));
    return SqlResult::ok();
}
