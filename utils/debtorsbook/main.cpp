#include <iostream>
#include <database/database.hpp>
#include <getopt.h>
#include <ctime>

static void setup_database(DB::DataBase &db)
{
    auto result = db.execute_sql("CREATE TABLE IF NOT EXISTS Debts ("
                   "    datetime BigInt,"
                   "    person Text,"
                   "    transaction Text,"
                   "    owedbyme Integer,"
                   "    owedbythem Integer)");

    if (!result.good())
        result.output_errors();
}

static void add_new_debt(DB::DataBase &db)
{
    std::string buffer;
    std::string name = "anon";
    std::string transaction = "donation";
    int owed_by_me = 0;
    int owed_by_them = 0;

    // Get input
    for (;;)
    {
        std::cout << "Name [" + name + "]: ";
        std::getline(std::cin, buffer);
        if (!buffer.empty())
            name = buffer;

        std::cout << "Transaction [" + transaction + "]: ";
        std::getline(std::cin, buffer);
        if (!buffer.empty())
            transaction = buffer;

        std::cout << "Owed by me [" + std::to_string(owed_by_me) + "]: ";
        std::getline(std::cin, buffer);
        if (!buffer.empty())
            owed_by_me = atoi(buffer.c_str());

        std::cout << "Owed by them [" + std::to_string(owed_by_them) + "]: ";
        std::getline(std::cin, buffer);
        if (!buffer.empty())
            owed_by_them = atoi(buffer.c_str());

        // Varify that's correct
        std::cout << "\n";
        std::cout << name << ": " << transaction << "\n";
        std::cout << "  Owed by me: " << owed_by_me << "\n";
        std::cout << "  Owed by them: " << owed_by_them << "\n";
        std::cout << "Is this correct [n]: ";
        std::getline(std::cin, buffer);

        if (buffer == "y" || buffer == "Y" || buffer == "yes" || buffer == "YES")
            break;
        std::cout << "\n";
    }
    std::cout << "\nAdding debt\n";

    // Add to database
    int64_t datetime = time(0);
    auto result = db.execute_sql("INSERT INTO Debts (datetime, person, transaction, owedbyme, owedbythem) VALUES ("
                   + std::to_string(datetime) + ", "
                   + "'" + name + "', "
                   + "'" + transaction + "', "
                   + std::to_string(owed_by_me) + ", "
                   + std::to_string(owed_by_them) + ")");

    if (!result.good())
        result.output_errors();
}

static void print_report(DB::DataBase &db)
{
    std::cout << "TODO: Implement this function\n";
}

static struct option cmd_options[] =
{
    { "help",       no_argument,        0, 'h' },
    { "add",        no_argument,        0, 'a' },
};

void show_help()
{
    std::cout << "usage: debtorsbook [-h]\n";
    std::cout << "\nManage small loans\n";
    std::cout << "\noptional arguments:\n";
    std::cout << "  -h, --help\t\tShow this help message and exit\n";
    std::cout << "  -a, --help\t\tAdd a new debt\n";
}

int main(int argc, char *argv[])
{
    enum class Mode
    {
        Default,
        Add,
    };

    auto mode = Mode::Default;
    for (;;)
    {
        int option_index;
        int c = getopt_long(argc, argv, "ha",
            cmd_options, &option_index);

        if (c == -1)
            break;

        auto set_mode = [&](Mode new_mode)
        {
            if (mode != Mode::Default)
            {
                show_help();
                exit(1);
            }

            mode = new_mode;
        };

        switch (c)
        {
            case 'h':
                show_help();
                return 0;
            case 'a':
                set_mode(Mode::Add);
                break;
        }
    }

    auto db = DB::DataBase::open("debtors_book.db");
    assert (db);
    setup_database(*db);

    switch (mode)
    {
        case Mode::Default:
            print_report(*db);
            break;
        case Mode::Add:
            add_new_debt(*db);
            break;
    }

    return 0;
}
