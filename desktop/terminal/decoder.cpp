#include "decoder.hpp"
#include "color.hpp"
#include <optional>
#include <iostream>

void Decoder::parse_attribute_code(int code,
    std::function<void(TerminalColor::Type, TerminalColor::Named)> on_color_callback)
{
    auto original_code = code;

    // Reset all attributes
    if (code == 0)
    {
        on_color_callback(TerminalColor::Background, TerminalColor::DefaultBackground);
        on_color_callback(TerminalColor::Foreground, TerminalColor::DefaultForeground);
        return;
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

    on_color_callback(type, color);
}

Decoder::Result Decoder::parse(char c)
{
    bool should_consume = true;
    do
    {
        should_consume = true;
        switch (m_state)
        {
            case State::Ascii:
                switch (c)
                {
                    case '\033':
                        m_current_args.clear();
                        m_current_argument.clear();
                        m_current_is_private = false;
                        m_state = State::Escape;
                        break;
                    
                    case 7:
                        return { Result::Bell, 0, {} };

                    case 8:
                        return
                        {
                            Result::Escape, 0,
                            { 'D', { 1 }, false }
                        };

                    case '\t':
                        return { Result::Tab, 0, {} };
                    
                    default:
                        return { Result::Rune, (uint32_t)c, {} };
                }
                break;
            
            case State::Escape:
                if (c == '[')
                {
                    m_state = State::EscapePrivate;
                    break;
                }

                // Invalid escape sequence
                m_state = State::Ascii;
                break;
            
            case State::EscapePrivate:
                if (c == '?')
                {
                    m_current_is_private = true;
                    m_state = State::EscapeArg;
                    break;
                }
                
                should_consume = false;
                m_state = State::EscapeArg;
                break;
            
            case State::EscapeArg:
                if (std::isdigit(c))
                {
                    m_current_argument += c;
                    break;
                }
                
                // Push argument if not empty
                if (!m_current_argument.empty())
                {
                    m_current_args.push_back(std::atoi(m_current_argument.c_str()));
                    m_current_argument.clear();
                }
                
                // Continue parsing the next argument
                if (c == ';')
                    break;
                
                m_state = State::EscapeCommand;
                should_consume = false;
                break;

            case State::EscapeCommand:
            {
                m_state = State::Ascii;
                
                auto sequence = EscapeSequence { c, m_current_args, m_current_is_private };
                return { Result::Escape, 0, sequence };
            }
        }
    } while (!should_consume);
    
    return { Result::Incomplete, 0, {} };
}
