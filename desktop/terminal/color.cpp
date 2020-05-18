#include "color.hpp"

bool TerminalColor::operator== (const TerminalColor &other) const
{
    return m_foreground == other.m_foreground &&
           m_background == other.m_background &&
           m_flags == other.m_flags;
}

TerminalColor TerminalColor::inverted() const
{
    return TerminalColor(m_background, m_foreground, m_flags);
}

static std::string color_to_string(TerminalColor::Named color)
{
    switch (color)
    {
        case TerminalColor::DefaultBackground: return "DefaultBackground";
        case TerminalColor::DefaultForeground: return "DefaultForeground";
        case TerminalColor::Black: return "Black";
        case TerminalColor::Red: return "Red";
        case TerminalColor::Green: return "Green";
        case TerminalColor::Yellow: return "Yellow";
        case TerminalColor::Blue: return "Blue";
        case TerminalColor::Magenta: return "Magenta";
        case TerminalColor::Cyan: return "Cyan";
        case TerminalColor::White: return "White";
    }

    return "Unkown(" + std::to_string(color) + ")";
}

std::string TerminalColor::name() const
{
    return "(Foreground = " + color_to_string(m_foreground) + 
        ", Background = " + color_to_string(m_background) + ")";
}

static int color_to_int(TerminalColor::Named color)
{
    auto to_int = [](const std::string &str) -> int
    {
        auto hex = str;
        if (str.length() <= 6)
            hex = "FF" + str;
        return std::stoul(hex, nullptr, 16);
    };
    
    switch(color)
    {
        case TerminalColor::DefaultBackground: return to_int(ColorPalette::DefaultBackground);
        case TerminalColor::DefaultForeground: return to_int(ColorPalette::DefaultForeground);
        case TerminalColor::Black: return to_int(ColorPalette::Black);
        case TerminalColor::Red: return to_int(ColorPalette::Red);
        case TerminalColor::Green: return to_int(ColorPalette::Green);
        case TerminalColor::Yellow: return to_int(ColorPalette::Yellow);
        case TerminalColor::Blue: return to_int(ColorPalette::Blue);
        case TerminalColor::Magenta: return to_int(ColorPalette::Magenta);
        case TerminalColor::Cyan: return to_int(ColorPalette::Cyan);
        case TerminalColor::White: return to_int(ColorPalette::White);
    }
    
    return 0;
}

int TerminalColor::foreground_int() const
{
    return color_to_int(m_foreground);
}

int TerminalColor::background_int() const
{
    return color_to_int(m_background);    
}
