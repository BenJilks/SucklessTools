#include <iostream>
#include <database/database.hpp>
#include <getopt.h>
#include <ctime>
#include <cassert>
#include <unistd.h>
#include <pwd.h>

static void setup_database(DB::DataBase &db)
{
    auto result = db.execute_sql("CREATE TABLE IF NOT EXISTS Debts ("
                   "    id Integer,"
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
    int id = rand();
    auto result = db.execute_sql("INSERT INTO Debts (id, datetime, person, transaction, owedbyme, owedbythem) VALUES ("
                   + std::to_string(id) + ", "
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

static std::optional<int> select_row(DB::DataBase &db)
{
    auto result = db.execute_sql("SELECT * FROM Debts");
    if (!result.good())
    {
        result.output_errors();
        return std::nullopt;
    }

    std::string buffer;
    std::string name_filter;
    std::string transaction_filter;
    int debt_selected = 0;

    auto update_filter = [&]()
    {
        std::string filter_by = "?";
        std::string term;

        for (;;)
        {
            std::cout << "\nFilter by [?]: ";
            std::getline(std::cin, buffer);
            if (!buffer.empty())
                filter_by = buffer;

            if (filter_by != "person" && filter_by != "transaction")
            {
                std::cout << "Options: person, transaction\n";
                continue;
            }
            break;
        }

        std::cout << "Term []: ";
        std::getline(std::cin, buffer);
        if (!buffer.empty())
        {
            if (filter_by == "person")
                name_filter = buffer;
            else if (filter_by == "transaction")
                transaction_filter = buffer;
        }
        std::cout << "\n";
    };

    std::vector<int> id_index;
    for (;;)
    {
        int id = 0;
        id_index.clear();
        for (const auto &row : result)
        {
            auto name = row["person"]->as_string();
            auto transaction = row["transaction"]->as_string();
            auto amount_owed_by_me = row["owedbyme"]->as_int();
            auto amount_owed_by_them = row["owedbythem"]->as_int();
            if ((!name_filter.empty() && name != name_filter)
                || (!transaction_filter.empty() && transaction != transaction_filter))
            {
                continue;
            }

            printf("%-3i: %-10s %-20s %-5i %-5i\n", id, name.c_str(),
                   transaction.c_str(), amount_owed_by_me, amount_owed_by_them);
            id_index.push_back(row["id"]->as_int());
            id += 1;
        }

        std::cout << "\nSelect a debt, type 'f' to filter [0]: ";
        std::getline(std::cin, buffer);
        if (buffer == "f" || buffer == "F")
            update_filter();
        else
            break;
    }

    if (!buffer.empty())
        debt_selected = atoi(buffer.c_str());

    return id_index[debt_selected];
}

static void pay_off_debt(DB::DataBase &db)
{
    auto selected_row = select_row(db);
    if (!selected_row)
        return;

    auto result = db.execute_sql("DELETE FROM Debts WHERE id="
        + std::to_string(*selected_row));
    if (!result.good())
        result.output_errors();
}

static struct option cmd_options[] =
{
    { "help",       no_argument,        0, 'h' },
    { "add",        no_argument,        0, 'a' },
    { "person",     required_argument,  0, 'p' },
    { "pay",        no_argument,        0, 'd' },
};

void show_help()
{
    std::cout << "usage: debtorsbook [-h] [-a] [-p <name>]\n";
    std::cout << "\nManage small loans\n";
    std::cout << "\noptional arguments:\n";
    std::cout << "  -h, --help\t\tShow this help message and exit\n";
    std::cout << "  -a, --add\t\tAdd a new debt\n";
    std::cout << "  -p, --person <name>\t\tShow debts of a person\n";
    std::cout << "  -d, --pay\t\tPay off a debt\n";
}

int main(int argc, char *argv[])
{
    // Initialize random
    srand(time(0));

    auto *pw = getpwuid(getuid());
    auto home_dir = pw->pw_dir;
    auto book_path = std::string(home_dir) + "/.debtors_book.db";

    std::string option;
    enum class Mode
    {
        Default,
        Add,
        Person,
        Pay,
    };

    auto mode = Mode::Default;
    for (;;)
    {
        int option_index;
        int c = getopt_long(argc, argv, "hap:d",
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
            case 'd':
                set_mode(Mode::Pay);
                break;
        }
    }

    auto db = DB::DataBase::open(book_path);
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
        case Mode::Pay:
            pay_off_debt(*db);
            break;
    }

    return 0;
}
