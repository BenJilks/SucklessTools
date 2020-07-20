#include "createtableifnotexists.hpp"
#include "../database.hpp"
using namespace DB;
using namespace DB::Sql;

SqlResult CreateTableIfNotExistsStatement::execute(DataBase &db) const
{
    auto *table = db.get_table(table_name());
    if (!table)
        return CreateTableStatement::execute(db);

    return SqlResult::ok();
}
