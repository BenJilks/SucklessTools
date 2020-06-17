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
        db.execute_sql(
            "CREATE TABLE " + table_name + " (" +
                "ID INTEGER," +
                "FirstName INTEGER," +
                "LastName INTEGER," +
                "Age INTEGER )");

        for (size_t i = 0; i < 10; i++)
        {
            db.execute_sql(
                "INSERT INTO " + table_name +
                    "(ID, FirstName, LastName, Age) " +
                    "VALUES (" +
                    std::to_string(i * 4 + 1) + ", " +
                    std::to_string(i * 4 + 2) + ", " +
                    std::to_string(i * 4 + 3) + ", " +
                    std::to_string(i * 4 + 4) + ")");
        }
    }
#else
    auto result = db.execute_sql("SELECT * FROM Test1");
    for (const auto &row : result)
        std::cout << row << "\n";

#endif

    return 0;
}
