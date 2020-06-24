#include "database.hpp"
#include "cleaner.hpp"
#include "prompt.hpp"
#include <iostream>
#include <cassert>
#include <getopt.h>
using namespace DB;

static struct option cmd_options[] =
{
    { "help",       no_argument,        0, 'h' },
    { "clean",      no_argument,        0, 'c' },
    { "info",       no_argument,        0, 'i' }
};

void show_help()
{
    std::cout << "usage: database [-h] [-c] [-i] <file>\n";
    std::cout << "\nManage databases\n";
    std::cout << "\noptional arguments:\n";
    std::cout << "  -h, --help\t\tShow this help message and exit\n";
    std::cout << "  -c, --clean\t\tClean up the database\n";
    std::cout << "  -i, --info\t\tOutput the internal structure\n";
}

int main(int argc, char *argv[])
{
    enum class Mode
    {
        Default,
        Clean,
        Info,
    };
    
    auto mode = Mode::Default;
    for (;;)
    {
        int option_index;
        int c = getopt_long(argc, argv, "hci",
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
            case 'c':
                if (mode_already_set())
                    return 1;
                mode = Mode::Clean;
                break;
            case 'i':
                if (mode_already_set())
                    return 1;
                mode = Mode::Info;
                break;
        }
    }

    if (optind != argc - 1)
    {
        show_help();
        return 1;
    }
    
    auto db_path = argv[optind];
    switch (mode)
    {
        case Mode::Default:
        {
            Prompt prompt(db_path);
            prompt.run();
            break;
        }
        case Mode::Clean:
        {
            Cleaner cleaner(db_path);
            cleaner.full_clean_up();
            break;
        }
        case Mode::Info:
        {
            Cleaner cleaner(db_path);
            cleaner.output_info();
            break;
        }
    }
    return 0;
}
