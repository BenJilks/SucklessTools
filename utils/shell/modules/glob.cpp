#include "glob.hpp"
#include <iostream>
#include <filesystem>

bool GlobModule::is_glob(const std::string &str)
{
    for (char c : str)
    {
        if (c == '?' || c == '*')
            return true;
    }

    return false;
}

bool GlobModule::hook_on_token(Lexer &lexer, Token &token, int)
{
    // Check that this token is a glob
    if (!is_glob(token.data))
        return false;

    std::vector<std::string> possible_paths;
    std::string current_file;

    if (token.data[0] == '/')
        possible_paths.push_back("/");
    else if (token.data[0] != '.')
        possible_paths.push_back(".");

    auto on_file_end = [&]()
    {
        std::vector<std::string> new_possible_paths;

        auto glob_options = resolve_glob(possible_paths, current_file);
        for (const auto &glob_option : glob_options)
        {
            for (const auto &possible_path : possible_paths)
                new_possible_paths.push_back(possible_path + "/" + glob_option);
        }

        possible_paths = new_possible_paths;
        current_file.clear();
    };

    bool is_first_char = true;
    for (char c : token.data)
    {
        if (c == '/')
        {
            if (!is_first_char)
                on_file_end();
            continue;
        }

        current_file += c;
        is_first_char = false;
    }

    if (!current_file.empty())
        on_file_end();

    // If no options were found, don't do anything
    if (possible_paths.size() == 0)
        return false;

    std::string options_out;
    for (const auto option : possible_paths)
    {
        if (option.size() >= 2 && option[0] == '/' && option[1] == '/')
            options_out += std::string(option.data() + 1, option.size() - 1) + " ";
        else if (option.size() >= 2 && option[0] == '.' && option[1] == '/')
            options_out += std::string(option.data() + 2, option.size() - 2) + " ";
        else
            options_out += option + " ";
    }
    lexer.insert_string(options_out);
    return true;
}

bool GlobModule::does_glob_match(const std::string &glob, const std::string &option)
{
    enum class State
    {
        Default,
        WildCard,
        Star,
    };

    auto state = State::Default;
    int option_index = 0;
    int glob_index = 0;

    bool should_exit = false;
    while (!should_exit)
    {
        switch (state)
        {
            case State::Default:
                if (glob_index >= (int)glob.size() || option_index >= (int)option.size())
                {
                    should_exit = true;
                    break;
                }

                switch (glob[glob_index])
                {
                    case '?':
                        state = State::WildCard;
                        break;

                    case '*':
                        state = State::Star;
                        break;

                    default:
                        if (option[option_index++] != glob[glob_index++])
                            return false;
                        break;
                }
                break;

            case State::WildCard:
                // Skip a char without checking
                option_index++;
                glob_index++;

                state = State::Default;
                break;

            case State::Star:
            {
                if (glob_index + 1 >= (int)glob.size())
                {
                    glob_index += 1;
                    option_index = option.size();
                    should_exit = true;
                    break;
                }

                // If we have more glob and we've reached the
                // end of the option string, it's not a match
                if (option_index >= (int)option.size())
                    return false;

                auto next_char_in_glob = glob[glob_index + 1];
                if (option[option_index++] == next_char_in_glob)
                {
                    glob_index += 2;
                    state = State::Default;
                    break;
                }
                break;
            }
        }
    }

    return glob_index >= (int)glob.size() && option_index >= (int)option.size();
}

std::vector<std::string> GlobModule::resolve_glob(const std::vector<std::string> &posible_paths, const std::string &glob)
{
    std::vector<std::string> out;
    for (const auto &possible_path : posible_paths)
    {
        if (!std::filesystem::is_directory(possible_path))
            continue;

        for (const auto &posible_file : std::filesystem::directory_iterator(possible_path))
        {
            const std::string &posible_file_name = posible_file.path().filename();
            if (does_glob_match(glob, posible_file_name))
                out.push_back(posible_file_name);
        }
    }

    return out;
}
