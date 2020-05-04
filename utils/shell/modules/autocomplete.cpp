#include "autocomplete.hpp"
#include "../shell.hpp"
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

std::string find_name(const std::string &str)
{
	int last_slash_index = 0;
	for (int i = str.length() - 1; i >= 0; i--)
	{
		if (str[i] == '/' || str[i] == '~')
		{
			last_slash_index = i + 1;
			break;
		}
	}

	return str.substr(last_slash_index, str.length() - last_slash_index);
}

bool AutoCompleteModule::hook_input(char c, 
	const std::string &line, int cursor,
	std::function<void(const std::string &)> insert,
	std::function<void(const std::string &)> replace,
	std::function<void(const std::string &)> message)
{
	if (c != '\t')
		return false;

	int word_start = cursor;
	while (word_start > 0)
	{
		char c = line[word_start - 1];
		if (std::isspace(c))
			break;

		word_start -= 1;
	}

	std::string_view curr_word(
		line.data() + word_start, 
		cursor - word_start);

	// Find all options for word
	auto options = find_completions(
		curr_word, word_start == 0);

	std::string patch = "";
	for (;;)
	{
		char next_char = 0;
		for (const auto &option : options)
		{
			int index = curr_word.length() + patch.length();
			if (index > option.length())
			{
				next_char = 0;
				break;
			}

			char c = option[index];
			if (next_char == 0)
				next_char = c;

			if (c != next_char)
			{
				next_char = 0;
				break;
			}

			next_char = c;
		}

		if (!next_char)
			break;

		patch += next_char;
	}

	insert(patch);
	if (options.size() == 1)
	{
		if (fs::is_directory(options[0]))
			insert("/");
		else
			insert(" ");
	}

	if (patch.empty())
	{
		if (options.size() > 0)
		{
			std::string suggestions;
			for (const auto &option : options)
				suggestions += find_name(option) + " ";
			message(suggestions);
		}
	}
	
	return true;
}

std::vector<std::string> AutoCompleteModule::find_completions(
	const std::string_view &start, bool search_path)
{
	std::vector<std::string> options;
	
	int last_slash_index = 0;
	for (int i = start.length() - 1; i >= 0; i--)
	{
		if (start[i] == '/' || start[i] == '~')
		{
			last_slash_index = i + 1;
			break;
		}
	}

	auto path = std::string(start.substr(0, last_slash_index));
	auto local_path = "./" + std::string(path);

	// If the path begins with a slash, it's an absolute path
	if (!path.empty())
	{
		if (path[0] == '/' || path[0] == '~')
			local_path = path;
	}

	// Look through local paths
	for (const auto &entry : fs::directory_iterator(Shell::the().expand_path(local_path)))
	{
		auto filename = std::string(path) + std::string(entry.path().filename());
		if (filename.rfind(start, 0) == 0)
			options.push_back(filename);
	}

	// If search path is enabled, look through all the paths in the envirement $PATH
	// variable for options
	if (search_path)
	{
		auto env_path = Shell::the().get("PATH");
		std::string buffer;
		
		for (int i = 0; i < env_path.length(); i++)
		{
			char c = env_path[i];
			if (c == ':')
			{
				for (const auto &entry : fs::directory_iterator(buffer))
				{
					auto filename = std::string(entry.path().filename());
					if (filename.rfind(start, 0) == 0)
						options.push_back(filename);
				}

				buffer = "";
				continue;
			}

			buffer += c;
		}
	}

	return options;
}

