#include "delete.hpp"
#include "value.hpp"
#include "../database.hpp"
using namespace DB;
using namespace DB::Sql;

SqlResult DeleteStatement::execute(DataBase &db) const
{
    assert (m_where);

    auto table = db.get_table(m_table);
    if (!table)
        return SqlResult::error("No table with the name '" + m_table + "' found");
    
    for (size_t i = 0; i < table->row_count(); i++)
    {
        auto row = table->get_row(i);
        assert (row);
        
        auto result = m_where->evaluate(*row);
        if (static_cast<ValueBoolean&>(*result).data())
            table->remove_row(i);
    }
    
    return SqlResult::ok();
}
