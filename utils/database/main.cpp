#include "database.hpp"
#include <iostream>
#include <cassert>
using namespace DB;

int main()
{
	std::cout << "Hello, databases!!!\n";

    auto db_or_error = DataBase::open("test.db");
    assert (db_or_error);

    auto &db = *db_or_error;

    /*
    Table::Constructor tc("Test");
    tc.add_column("ID", DataType::integer());
    tc.add_column("FirstName", DataType::integer());
    tc.add_column("LastName", DataType::integer());
    tc.add_column("Age", DataType::integer());

    auto &table = db.construct_table(tc);
    for (size_t i = 0; i < 10; i++)
    {
        auto rc = table.new_row();
        rc.integer_entry(i * 4 + 1);
        rc.integer_entry(i * 4 + 2);
        rc.integer_entry(i * 4 + 3);
        rc.integer_entry(i * 4 + 4);
        table.add_row(std::move(rc));
    }
    */

    auto table_or_error = db.get_table("Test");
    assert (table_or_error);

    auto &table = *table_or_error;
    std::cout << *table.get_row(0) << "\n";
    return 0;
}
