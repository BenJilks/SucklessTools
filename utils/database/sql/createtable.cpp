#include "createtable.hpp"
#include "../table.hpp"
#include "../database.hpp"
#include <algorithm>
#include <cassert>
using namespace DB;
using namespace DB::Sql;

SqlResult CreateTableStatement::execute(DataBase& db) const
{
    if (db.get_table(m_name))
        return SqlResult::error("Table with the name '" + m_name + "' already exists");
    
    Table::Constructor tc(m_name);
    for (const auto &column : m_columns)
    {
        auto type_name = column.type;
        std::for_each(type_name.begin(), type_name.end(), [](char &c)
        {
            c = ::tolower(c);
        });

        std::optional<DataType> type;
        if (type_name == "integer")
            type = DataType::integer();

        assert (type);
        tc.add_column(column.name, *type);
    }

    db.construct_table(tc);
    return SqlResult::ok();
}
