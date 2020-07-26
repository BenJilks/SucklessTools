#pragma once
#include <string>

namespace Imap
{

    class Parser
    {
    public:
        Parser(const std::string &in);

        std::string parse_type();
        std::vector<std::string> parse_flags();

        std::string parse_string();
        int parse_number();
        void parse_char(char c);

        void next_line();
        bool is_eof();

    private:
        const std::string &m_in;
        int m_index { 0 };

        void skip_white_space();

    };

}
