#include <iostream>
#include <database/database.hpp>
#include <getopt.h>

static void setup_database(DB::DataBase &db)
{
    db.execute_sql("CREATE TABLE IF NOT EXISTS Debts ("
                   "    datetime BigInt"
                   "    person Text"
                   "    transaction Text"
                   "    owedbyme Int"
                   "    owedbythem Int)");
}

static struct option cmd_options[] =
{
    { "help",       no_argument,        0, 'h' },
};

void show_help()
{
    std::cout << "usage: debtorsbook [-h]\n";
    std::cout << "\nManage small loans\n";
    std::cout << "\noptional arguments:\n";
    std::cout << "  -h, --help\t\tShow this help message and exit\n";
}

int main(int argc, char *argv[])
{
    enum class Mode
    {
        Default,
    };

    auto mode = Mode::Default;
    for (;;)
    {
        int option_index;
        int c = getopt_long(argc, argv, "h",
            cmd_options, &option_index);

        if (c == -1)
            break;

        auto mode_already_set = [&]()
        {
            if (mode != Mode::Default)
            {
                show_help();
                return true;
            }

            return false;
        };

        switch (c)
        {
            case 'h':
                show_help();
                return 0;
        }
    }

    auto db = DB::DataBase::open("debtors_book.db");
    assert (db);

    setup_database(*db);
    return 0;
}
