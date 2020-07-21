#include <iostream>
#include <database/database.hpp>
#include <getopt.h>
#include <ctime>
#include <cassert>

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

static void print_report(DB::DataBase &db, const std::string &condition = "")
{
    auto result = db.execute_sql("SELECT * FROM Debts " + condition);
    if (!result.good())
    {
        result.output_errors();
        return;
    }

    int total_i_owe = 0;
    int total_owed_to_me = 0;
    printf("-----------------------------------------------------\n");
    printf("| %-10s | %-20s | %-5s | %-5s |\n", "Person", "Transaction", "Me", "Them");
    printf("| ---------- | -------------------- | ----- | ----- |\n");
    for (const auto &row : result)
    {
        auto name = row["person"]->as_string();
        auto transaction = row["transaction"]->as_string();
        auto amount_owed_by_me = row["owedbyme"]->as_int();
        auto amount_owed_by_them = row["owedbythem"]->as_int();
        printf("| %-10s | %-20s | %-5i | %-5i |\n", name.c_str(),
               transaction.c_str(), amount_owed_by_me, amount_owed_by_them);

        total_i_owe += amount_owed_by_me;
        total_owed_to_me += amount_owed_by_them;
    }
    printf("-----------------------------------------------------\n");

    printf("\n%-20s %-10i\n", "Total I Owe: ", total_i_owe);
    printf("%-20s %-10i\n", "Total Owed to Me: ", total_owed_to_me);
    printf("%-20s %-10i\n", "Total Over Due: ", total_owed_to_me - total_i_owe);
}

static struct option cmd_options[] =
{
    { "help",       no_argument,        0, 'h' },
    { "add",        no_argument,        0, 'a' },
    { "person",     no_argument,        0, 'p' },
};

void show_help()
{
    std::cout << "usage: debtorsbook [-h] [-a] [-p <name>]\n";
    std::cout << "\nManage small loans\n";
    std::cout << "\noptional arguments:\n";
    std::cout << "  -h, --help\t\tShow this help message and exit\n";
    std::cout << "  -a, --add\t\tAdd a new debt\n";
    std::cout << "  -p, --person <name>\t\tShow debts of a person\n";
}

int main(int argc, char *argv[])
{
    std::string option;
    enum class Mode
    {
        Default,
        Add,
        Person,
    };

    auto mode = Mode::Default;
    for (;;)
    {
        int option_index;
        int c = getopt_long(argc, argv, "hap:",
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
            case 'p':
                set_mode(Mode::Person);
                option = optarg;
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
        case Mode::Person:
            print_report(*db, "WHERE person='" + option + "'");
            break;
    }

    return 0;
}
