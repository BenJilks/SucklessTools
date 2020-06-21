#include "database.hpp"
#include "cleaner.hpp"
#include <iostream>
#include <cassert>
using namespace DB;

int main()
{
    std::cout << "Hello, databases!!!\n";
    Cleaner cleaner("test.db");
    cleaner.full_clean_up();

    auto db_or_error = DataBase::open("test_cleaned.db");
    assert (db_or_error);

    auto &db = *db_or_error;

#if 0
    for (size_t i = 0; i < 2; i++)
    {
        auto table_name = "Test" + std::to_string(i);
        db.execute_sql(
            "CREATE TABLE " + table_name + " (" +
                "ID INTEGER," +
                "FirstName TEXT," +
                "LastName CHAR(80)," +
                "Age INTEGER )");

        for (size_t i = 0; i < 10; i++)
        {
            db.execute_sql(
                "INSERT INTO " + table_name +
                    "(ID, FirstName, Age) " +
                    "VALUES (" +
                    std::to_string(i * 4 + 1) + ", " +
                    "'bob', " +
                    std::to_string(i * 4 + 4) + ")");
        }
    }

    {
        auto result = db.execute_sql("UPDATE Test1 SET Age = 69 WHERE ID = 17");
        if (!result.good())
            result.output_errors();
    }

    {
        auto result = db.execute_sql("DELETE FROM Test1 WHERE ID = 21");
        if (!result.good())
            result.output_errors();
    }

#else
    {
        auto result = db.execute_sql("SELECT * FROM Test0");
        if (!result.good())
            result.output_errors();

        for (const auto &row : result)
            std::cout << row << "\n";
    }

#endif

    return 0;
}
