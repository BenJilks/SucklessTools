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

#if 0
    for (size_t i = 0; i < 2; i++)
    {
        Table::Constructor tc("Test" + std::to_string(i));
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
    }
#else
    auto table0 = db.get_table("Test0");
    auto table1 = db.get_table("Test1");
    assert (table0);
    assert (table1);

#if 0
    for (size_t i = 0; i < 2; i++)
    {
        auto rc = table1->new_row();
        rc.integer_entry(0);
        rc.integer_entry(1);
        rc.integer_entry(2);
        rc.integer_entry(3);
        table1->add_row(std::move(rc));
    }
#endif

    std::cout << *(*table1).get_row(10) << "\n";
#endif

    return 0;
}
