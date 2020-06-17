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
        auto table_name = "Test" + std::to_string(i);
        
        Table::Constructor tc(table_name);
        tc.add_column("ID", DataType::integer());
        tc.add_column("FirstName", DataType::integer());
        tc.add_column("LastName", DataType::integer());
        tc.add_column("Age", DataType::integer());

        auto &table = db.construct_table(tc);
        for (size_t i = 0; i < 10; i++)
        {
            db.execute_sql("INSERT INTO " + table_name + 
                " (ID, FirstName, LastName, Age) " + 
                    "VALUES (" + 
                    std::to_string(i * 4 + 1) + ", " +
                    std::to_string(i * 4 + 2) + ", " +
                    std::to_string(i * 4 + 3) + ", " +
                    std::to_string(i * 4 + 4) + ")");
/*
            auto row = table.make_row();
            row["ID"] = std::make_unique<IntegerEntry>(i * 4 + 1);
            row["FirstName"] = std::make_unique<IntegerEntry>(i * 4 + 2);
            row["LastName"] = std::make_unique<IntegerEntry>(i * 4 + 3);
            row["Age"] = std::make_unique<IntegerEntry>(i * 4 + 4);
            table.add_row(std::move(row));
*/
        }
    }
#else
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

    auto table = db.get_table("Test1");
    assert (table);
    
    auto row = table->get_row(0);
    assert (row);
    
    std::cout << *row << "\n";
    
#endif

    return 0;
}
