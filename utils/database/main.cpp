#include "database.hpp"
#include "cleaner.hpp"
#include "prompt.hpp"
#include <iostream>
#include <cassert>
#include <getopt.h>
using namespace DB;

static struct option cmd_options[] =
{
    { "help",       no_argument,        0, 'h' }
};

void show_help()
{
    std::cout << "usage: database [-h] <file>\n";
    std::cout << "\nManage databases\n";
    std::cout << "\noptional arguments:\n";
    std::cout << "  -h, --help\t\tShow this help message and exit\n";
}

int main(int argc, char *argv[])
{
    for (;;)
    {
        int option_index;
        int c = getopt_long(argc, argv, "h",
            cmd_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                show_help();
                return 0;
        }
    }

    if (optind != argc - 1)
    {
        show_help();
        return 1;
    }
    
    Prompt prompt(argv[optind]);
    prompt.run();
    return 0;
}

int main_test()
{
    std::cout << "Hello, databases!!!\n";

    auto db_or_error = DataBase::open("test.db");
    assert (db_or_error);

    auto &db = *db_or_error;

#if 1
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

//#else
    {
        auto result = db.execute_sql("SELECT * FROM Test1");
        if (!result.good())
            result.output_errors();

        for (const auto &row : result)
            std::cout << row << "\n";
    }

#endif

    return 0;
}
