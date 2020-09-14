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
        EscapeBracket,
        OSCommandStart,
        OSCommandBody,
        Utf8OneByteLeft,
        Utf8TwoBytesLeft,
        Utf8ThreeBytesLeft,
    };

    struct EscapeSequence
    {
        char command;
        std::vector<int> args;
        bool is_private;
    };

    struct OSCommand
    {
        int command;
        std::string body;
    };

    struct Result
    {
        enum Type
        {
            Incomplete,
            Rune,
            Escape,
            OSCommand,
            Bell,
            Tab,
        };

        Type type;
        uint32_t value;
        struct EscapeSequence escape;
        struct OSCommand os_command;
    };

    Decoder() {}
    
    Result parse(char c);
    
private:
    State m_state { State::Ascii };
    uint32_t m_current_rune { 0 };
    std::string m_current_argument;
    std::vector<int> m_current_args;
    bool m_current_is_private;
    
};
