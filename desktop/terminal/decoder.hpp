#pragma once
#include "color.hpp"
#include <stdint.h>
#include <variant>
#include <string>
#include <vector>
#include <functional>

class Decoder
{
public:
    enum class State
    {
        Ascii,
        Escape,
        EscapePrivate,
        EscapeArg,
        EscapeCommand,
        EscapeHash,
    };    

    struct EscapeSequence
    {
        char command;
        std::vector<int> args;
        bool is_private;
    };

    struct Result
    {
        enum Type
        {
            Incomplete,
            Rune,
            Escape,
            Bell,
            Tab,
        };

        Type type;
        uint32_t value;
        EscapeSequence escape;
    };

    Decoder() {}
    
    Result parse(char c);
    
private:
    State m_state { State::Ascii };
    std::string m_current_argument;
    std::vector<int> m_current_args;
    bool m_current_is_private;
    
};
