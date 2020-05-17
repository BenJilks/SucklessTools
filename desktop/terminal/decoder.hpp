#pragma once
#include <stdint.h>
#include <variant>
#include "escapes.hpp"

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
    };
    
    struct Result
    {
        enum Type
        {
            Incomplete,
            Rune,
            Escape
        };

        Type type;
        std::variant<uint32_t, std::unique_ptr<Escape::Sequence>> value;
    };
    
    Decoder() {}
    
    Result parse(char c);
    
private:
    State m_state { State::Ascii };
    std::string m_current_argument;
    std::vector<int> m_current_args;
    bool m_current_is_private;
    
};
