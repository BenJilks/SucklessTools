#include <iostream>
#include <fstream>
#include <database/database.hpp>
#include <libconfig.hpp>
#include <getopt.h>
#include <ctime>
#include <cassert>
#include <unistd.h>
#include <pwd.h>

static std::string g_book = "~/.debtors_book.db";

static void setup_database(DB::DataBase &db)
{
    auto result = db.execute_sql("CREATE TABLE IF NOT EXISTS Debts ("
                   "    id Integer,"
                   "    datetime BigInt,"
                   "    person Char(80),"
                   "    transaction Char(80),"
                   "    owedbyme Float,"
                   "    owedbythem Float)");

    if (!result.good())
        result.output_errors();
}

static void add_new_debt(DB::DataBase &db, bool multi_mode)
{
    std::string buffer;
    std::string name = "anon";
    std::string transaction = "donation";
    float owed_by_me = 0;
    float owed_by_them = 0;

    do
    {

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
                owed_by_me = atof(buffer.c_str());

            std::cout << "Owed by them [" + std::to_string(owed_by_them) + "]: ";
            std::getline(std::cin, buffer);
            if (!buffer.empty())
                owed_by_them = atof(buffer.c_str());

            // Varify that's correct
            std::cout << "\n";
            std::cout << name << ": " << transaction << "\n";
            std::cout << "  Owed by me: " << owed_by_me << "\n";
            std::cout << "  Owed by them: " << owed_by_them << "\n";
            std::cout << "Is this correct [y]: ";
            std::getline(std::cin, buffer);

            if (buffer != "n" && buffer != "N" && buffer != "no" && buffer != "No")
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

    } while (multi_mode);
}

static void print_report(DB::DataBase &db, const std::string &condition = "")
{
    auto result = db.execute_sql("SELECT * FROM Debts " + condition);
    if (!result.good())
    {
        result.output_errors();
        return;
    }

    float total_i_owe = 0;
    float total_owed_to_me = 0;
    printf("-----------------------------------------------------------------\n");
    printf("| %-10s | %-20s | %-11s | %-11s |\n", "Person", "Transaction", "Me", "Them");
    printf("| ---------- | -------------------- | ----------- | ----------- |\n");
    for (const auto &row : result)
    {
        auto name = row["person"]->as_string();
        auto transaction = row["transaction"]->as_string();
        auto amount_owed_by_me = row["owedbyme"]->as_float();
        auto amount_owed_by_them = row["owedbythem"]->as_float();
        printf("| %-10s | %-20s | £%-10.2f | £%-10.2f |\n", name.c_str(),
               transaction.c_str(), amount_owed_by_me, amount_owed_by_them);

        total_i_owe += amount_owed_by_me;
        total_owed_to_me += amount_owed_by_them;
    }
    printf("-----------------------------------------------------------------\n");

    printf("\n%-20s £%-10.2f\n", "Total I Owe: ", total_i_owe);
    printf("%-20s £%-10.2f\n", "Total Owed to Me: ", total_owed_to_me);
    printf("%-20s £%-10.2f\n", "Total Over Due: ", total_owed_to_me - total_i_owe);
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
            auto amount_owed_by_me = row["owedbyme"]->as_float();
            auto amount_owed_by_them = row["owedbythem"]->as_float();
            if ((!name_filter.empty() && name != name_filter)
                || (!transaction_filter.empty() && transaction != transaction_filter))
            {
                continue;
            }

            printf("%-3i: %-10s %-20s £%-5.2f £%-5.2f\n", id, name.c_str(),
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

static void pay_off_debt(DB::DataBase &db, bool multi_mode)
{
    do
    {
        auto selected_row = select_row(db);
        if (!selected_row)
            return;

        auto result = db.execute_sql("DELETE FROM Debts WHERE id="
            + std::to_string(*selected_row));
        if (!result.good())
            result.output_errors();
    } while (multi_mode);
}

static struct option cmd_options[] =
{
    { "help",       no_argument,        0, 'h' },
    { "add",        no_argument,        0, 'a' },
    { "person",     required_argument,  0, 'p' },
    { "pay",        no_argument,        0, 'd' },
    { "multi",      no_argument,        0, 'm' },
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
    std::cout << "  -m, --multi\t\tDo multiple at a time\n";
}

int main(int argc, char *argv[])
{
    // Initialize random
    srand(time(0));

    std::string config_path = Config::resolve_home_path("~/.config/debtorsbook.conf");
    Config::load(config_path, [&](auto name, auto value)
    {
        if (name == "book")
            g_book = value;
    });

    std::string option;
    enum class Mode
    {
        Default,
        Add,
        Person,
        Pay,
    };

    auto multi_mode = false;
    auto mode = Mode::Default;
    for (;;)
    {
        int option_index;
        int c = getopt_long(argc, argv, "hap:dm",
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
            case 'm':
                multi_mode = true;
                break;
        }
    }

    auto db = DB::DataBase::open(Config::resolve_home_path(g_book));
    assert (db);
    setup_database(*db);

    switch (mode)
    {
        case Mode::Default:
            print_report(*db);
            break;
        case Mode::Person:
            print_report(*db, "WHERE person='" + option + "'");
            break;
        case Mode::Add:
            add_new_debt(*db, multi_mode);
            break;
        case Mode::Pay:
            pay_off_debt(*db, multi_mode);
            break;
    }

    return 0;
}
