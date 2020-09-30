#include "attribute.hpp"
#include <iostream>

void Attribute::apply(int code)
{
    auto original_code = code;

    switch (code)
    {
        // Reset all attributes
        case 0:
            m_color.set_background(TerminalColor::DefaultBackground);
            m_color.set_foreground(TerminalColor::DefaultForeground);
            m_flags = 0;
            return;

        case 1:
            m_flags |= Flag::Bold;
            return;

        case 7:
        case 27:
            m_color = m_color.inverted();
            return;

        default:
            break;
    }

    auto type = TerminalColor::Foreground;
    auto color = TerminalColor::DefaultForeground;
    if (code >= 90)
    {
        // TODO: Bright
        code -= 60;
    }

    code -= 30;
    if (code >= 10)
    {
        type = TerminalColor::Background;
        code -= 10;
    }

    switch (code)
    {
        case 0: color = TerminalColor::Black; break;
        case 1: color = TerminalColor::Red; break;
        case 2: color = TerminalColor::Green; break;
        case 3: color = TerminalColor::Yellow; break;
        case 4: color = TerminalColor::Blue; break;
        case 5: color = TerminalColor::Magenta; break;
        case 6: color = TerminalColor::Cyan; break;
        case 7: color = TerminalColor::White; break;
        case 8:
        case 9:
            if (type == TerminalColor::Background)
                color = TerminalColor::DefaultBackground;
            else
                color = TerminalColor::DefaultForeground;
            break;
        default:
            std::cout << "Invalid attribute: " << original_code << "\n";
            return;
    }

    m_color.from_type(type) = color;
}
