#include "insert.hpp"
#include "../database.hpp"
using namespace DB;
using namespace DB::Sql;

SqlResult InsertStatement::execute(DataBase& db) const
{
    if (m_columns.size() != m_values.size())
    {
        assert (false);
        return {};
    }
    
    auto table = db.get_table(m_table);
    if (!table)
        return {};
    
    auto row = table->make_row();
    for (int i = 0; i < m_columns.size(); i++)
    {
        const auto &column = m_columns[i];
        const auto &value = m_values[i];
        
        switch (value.type())
        {
            case Value::Integer:
                row[column] = std::make_unique<IntegerEntry>(value.as_int());
                break;
            
            default:
                assert (false);
        }
    }
    
    table->add_row(std::move(row));
    return {};
}
