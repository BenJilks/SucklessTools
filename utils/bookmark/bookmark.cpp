#include <libjson.hpp>
#include <map>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <unistd.h>

static struct option cmd_options[] =
{
	{ "help",	no_argument,		0, 'h' },
	{ "add",	required_argument, 	0, 'a' },
	{ "remove", required_argument, 	0, 'r' },
	{ "goto",   required_argument, 	0, 'g' },
	{ "list",	no_argument,		0, 'l' }
};

void show_help()
{
	std::cout << "usage: bookmark [-h] [-l] [-a Add] [-r Remove] [-g Goto]\n";
	std::cout << "\nManage bookmarks\n";
	std::cout << "\noptional arguments:\n";
	std::cout << "  -h, --help\t\tShow this help message and exit\n";
	std::cout << "  -l, --list\t\tShow saved bookmarks\n";
	std::cout << "  -a, --add\t\tAdd this directory as a bookmark\n";
	std::cout << "  -r, --remove\t\tRemove this directory from bookmarks\n";
	std::cout << "  -g, --goto\t\tGo to a bookmark\n";
}

int main(int argc, char *argv[])
{
	auto bookmarks_path = std::string(getenv("HOME")) + "/.bookmarks";
	auto doc = Json::Document::parse(std::ifstream(bookmarks_path));
    auto root = doc.root();
    if (doc.has_error())
        doc.log_errors();

	if (!root)
		root = std::make_shared<Json::Object>();

	auto bookmarks = root->as<Json::Object>();
	if (!bookmarks)
	{
		root = std::make_shared<Json::Object>();
		bookmarks = root->as<Json::Object>();
	}

	bool should_list = true;
	for (;;)
	{
		int option_index;
		int c = getopt_long(argc, argv, "ha:r:g:l",
			cmd_options, &option_index);

		if (c == -1)
			break;

		should_list = false;
		switch (c)
		{
			case 'h':
				show_help();
				return 0;

			case 'l':
				should_list = true;
				break;

			case 'a':
			{
				bookmarks->add(optarg, getenv("PWD"));
				break;
			}

			case 'r':
				if (!bookmarks->contains(optarg))
				{
					std::cerr << "Error: No bookmark called '" << optarg << "'\n";
					exit(-1);
				}

				bookmarks->remove(optarg);
				break;

			case 'g':
			{
				auto dir = bookmarks->get_as<Json::String>(optarg);
				if (!dir)
				{
					std::cerr << "Error: No bookmark called '" << optarg << "'\n";
					exit(-1);
				}

				chdir(dir->get().c_str());
				system(getenv("SHELL"));
				exit(0);
			}
		}
	}

	if (should_list)
	{
		for (auto value : *bookmarks)
			std::cout << value.first << " -> " << value.second << "\n";
	}

	std::ofstream out(bookmarks_path);
	out << root->to_string(Json::PrintOption::PrettyPrint) << "\n";
	out.close();

	return 0;
}
