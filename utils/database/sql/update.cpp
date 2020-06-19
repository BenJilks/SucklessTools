#include "update.hpp"
#include "value.hpp"
#include "../database.hpp"
#include <cassert>
using namespace DB;
using namespace DB::Sql;

SqlResult UpdateStatement::execute(DataBase &db) const
{
    auto table = db.get_table(m_table);
    if (!table)
        return SqlResult::error("No table the the name '" + m_table + "' found");

    auto execute_assignments_on_row = [&](size_t index, Row &row)
    {
        for (const auto &column : m_columns)
            row[column.column] = column.value->as_entry();
        
        table->update_row(index, std::move(row));
    };
    
    for (size_t i = 0; i < table->row_count(); i++)
    {
        auto row = table->get_row(i);
        assert (row);
        
        if (!m_where)
        {
            execute_assignments_on_row(i, *row);
            continue;
        }
        
        auto result = m_where->evaluate(*row);
        assert (result);
            
        if (static_cast<const ValueBoolean&>(*result).data())
            execute_assignments_on_row(i, *row);
    }
    
    return SqlResult::ok();
}
