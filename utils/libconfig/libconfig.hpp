#pragma once
#include <map>
#include <string>
#include <fstream>
#include <unistd.h>
#include <pwd.h>

namespace Config
{

    template<typename StringType>
    static std::string_view trim(const StringType &str)
    {
        int start = 0;
        int end = str.length();
        for (char c : str)
        {
            if (!isspace(c))
                break;
            start += 1;
        }

        for (int i = str.length(); i >= 0; i--)
        {
            if (!isspace(str[i]))
                break;
            end -= 1;
        }

        return std::string_view(str.data() + start, end - start);
    }

    template<typename StringType>
    static std::string resolve_home_path(const StringType &relative_path)
    {
        std::string path = relative_path;
        if (std::string_view(&relative_path[0]).empty())
            return {};

        if (relative_path[0] == '~')
        {
            // Cache home dir
            static struct passwd *pw;
            static const char *home_dir;
            if (!pw)
            {
                pw = getpwuid(getuid());
                home_dir = pw->pw_dir;
            }

            path = home_dir + std::string(&relative_path[0] + 1);
        }

        return path;
    }

    template<typename CallbackFunc>
    static void load(const std::string &file_path, CallbackFunc callback)
    {
        std::ifstream config(file_path);
        if (!config.good())
            return;

        std::string line;
        while (std::getline(config, line))
        {
            auto line_trimmed = trim(line);

            // Skip empty lines
            if (line_trimmed.empty())
                continue;

            // Skip comments
            if (line_trimmed[0] == '#')
                continue;

            int name_len = 0;
            for (char c : line)
            {
                if (c == '=')
                    break;
                name_len += 1;
            }

            auto name = std::string_view(line.data(), name_len);
            auto value = std::string_view(line.data() + name_len + 1, line.length() - name_len - 1);
            callback(trim(name), trim(value));
        }
    }

}
