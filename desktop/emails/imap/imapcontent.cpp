#include "imapcontent.hpp"
#include <iostream>
using namespace Imap;

enum class State
{
    Key,
    BeforeValue,
    Value,
    ValueNextLine,
    Content,
};

bool Content::parse()
{
    auto state = State::Key;
    std::cout << m_in << "\n";
    std::cout << "\n\n\n";

    std::string key, value, content;
    bool is_eof = false;
    int index = 0;
    while (!is_eof)
    {
        if (index >= (int)m_in.size())
            break;

        char c = m_in[index++];
        switch (state)
        {
            case State::Key:
                if (c == ':')
                {
                    state = State::BeforeValue;
                    break;
                }

                if (index >= (int)m_in.size())
                {
                    is_eof = true;
                    break;
                }

                key += c;
                break;

            case State::BeforeValue:
                if (isspace(c))
                    break;

                index -= 1;
                state = State::Value;
                break;

            case State::Value:
                if (c == '\n' || index >= (int)m_in.size())
                {
                    state = State::ValueNextLine;
                    break;
                }

                value += c;
                break;

            case State::ValueNextLine:
                if (c == '\r')
                {
                    state = State::Content;
                    break;
                }

                if (isspace(c))
                {
                    value += '\n';
                    value += c;
                    state = State::Value;
                    break;
                }

                index -= 1;
                state = State::Key;
                std::cout << key << " = " << value << "\n";
                key = "";
                value = "";
                break;

            case State::Content:
                break;
        }
    }

    return true;
}
