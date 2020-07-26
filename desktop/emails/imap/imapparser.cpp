#include "imapparser.hpp"
using namespace Imap;

Parser::Parser(const std::string &in)
    : m_in(in)
{
    // Skip '*'
    skip_white_space();
    assert (!is_eof() && m_in[m_index++] == '*');
}

std::string Parser::parse_type()
{
    std::string type;

    skip_white_space();
    while (!is_eof() && !isspace(m_in[m_index]))
        type += m_in[m_index++];

    return type;
}

std::vector<std::string> Parser::parse_flags()
{
    std::vector<std::string> flags;

    skip_white_space();
    assert (!is_eof() && m_in[m_index++] == '(');

    skip_white_space();
    for (;;)
    {
        if (!is_eof() && m_in[m_index] == ')')
            break;
        assert (!is_eof() && m_in[m_index++] == '\\');

        std::string flag;
        while (!is_eof() && isalnum(m_in[m_index]))
            flag += m_in[m_index++];
        skip_white_space();

        flags.push_back(flag);
    }

    assert (!is_eof() && m_in[m_index++] == ')');
    return flags;
}

std::string Parser::parse_string()
{
    std::string str;

    skip_white_space();
    assert (!is_eof() && m_in[m_index++] == '"');
    while (!is_eof() && m_in[m_index] != '"')
        str.push_back(m_in[m_index++]);
    assert (!is_eof() && m_in[m_index++] == '"');

    return str;
}

int Parser::parse_number()
{
    std::string buffer;

    skip_white_space();
    while (!is_eof() && isdigit(m_in[m_index]))
        buffer.push_back(m_in[m_index++]);

    return atoi(buffer.c_str());
}

void Parser::parse_char(char c)
{
    skip_white_space();
    assert (!is_eof() && m_in[m_index++] == c);
}

void Parser::next_line()
{
    // Skip to '\n'
    while (!is_eof() && m_in[m_index] != '\n')
        m_index += 1;

    // Skil the actual '\n'
    m_index += 1;
}

bool Parser::is_eof()
{
    return m_index >= (int)m_in.size();
}

void Parser::skip_white_space()
{
    while (!is_eof() && isspace(m_in[m_index]))
        m_index += 1;
}
