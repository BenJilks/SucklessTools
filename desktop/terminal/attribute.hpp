#pragma once
#include "color.hpp"

class Attribute
{
public:
    enum Flag
    {
        None = 0 << 0,
        Bold = 1 << 0,
    };

    Attribute() = default;

    void apply(int code);

    inline const TerminalColor &color() const { return m_color; }
    inline int flags() const { return m_flags; }

private:
    TerminalColor m_color { TerminalColor::default_color() };
    int m_flags { 0 };

};
