#include "color.hpp"

TerminalColor TerminalColor::inverted() const
{
    return TerminalColor(m_background, m_foreground, m_flags);
}

static std::string color_to_string(TerminalColor::Named color)
{
    switch (color)
    {
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
    switch(color)
    {
        case TerminalColor::Black: return std::stoul(ColorPalette::Black, nullptr, 16);
        case TerminalColor::Red: return std::stoul(ColorPalette::Red, nullptr, 16);
        case TerminalColor::Green: return std::stoul(ColorPalette::Green, nullptr, 16);
        case TerminalColor::Yellow: return std::stoul(ColorPalette::Yellow, nullptr, 16);
        case TerminalColor::Blue: return std::stoul(ColorPalette::Blue, nullptr, 16);
        case TerminalColor::Magenta: return std::stoul(ColorPalette::Magenta, nullptr, 16);
        case TerminalColor::Cyan: return std::stoul(ColorPalette::Cyan, nullptr, 16);
        case TerminalColor::White: return std::stoul(ColorPalette::White, nullptr, 16);
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
