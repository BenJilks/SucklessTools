#include "decoder.hpp"
#include "color.hpp"
#include "buffer.hpp"
#include <optional>
#include <iostream>
#include <bitset>

Decoder::Result Decoder::parse(char c)
{
#if 0
    if (c == '\033')
        std::cout << "\\033";
    else
        std::cout << c;
    std::cout.flush();
#endif

    bool should_consume = true;
    do
    {
        should_consume = true;
        switch (m_state)
        {
            case State::Ascii:
                if ((c & 0b11100000) == 0b11000000)
                {
                    m_current_rune = (c & 0b00011111) << 6;
                    m_state = State::Utf8OneByteLeft;
                    break;
                }
                if ((c & 0b11110000) == 0b11100000)
                {
                    m_current_rune = (c & 0b00001111) << 12;
                    m_state = State::Utf8TwoBytesLeft;
                    break;
                }
                if ((c & 0b11111000) == 0b11110000)
                {
                    m_current_rune = (c & 0b00000111) << 18;
                    m_state = State::Utf8ThreeBytesLeft;
                    break;
                }

                switch (c)
                {
                    case '\0':
                        break;

                    case '\033':
                        m_current_args.clear();
                        m_current_argument.clear();
                        m_current_is_private = false;
                        m_state = State::Escape;
                        break;
                    
                    case 7:
                        return { Result::Bell, 0, {}, {} };

                    case 8:
                        return
                        {
                            Result::Escape, 0,
                            { 'D', { 1 }, false },
                            {}
                        };

                    case '\r':
                        return { Result::Escape, 0, { 'G', {}, false }, {} };

                    case '\t':
                        return { Result::Tab, 0, {}, {} };
                    
                    default:
                        return { Result::Rune, (uint32_t)c, {}, {} };
                }
                break;
            
            case State::Escape:
                if (c == '[')
                {
                    m_state = State::EscapePrivate;
                    break;
                }

                if (c == ']')
                {
                    m_state = State::OSCommandStart;
                    break;
                }

                if (c == '#')
                {
                    m_state = State::EscapeHash;
                    break;
                }

                if (c == '(')
                {
                    m_state = State::EscapeBracket;
                    break;
                }

                m_state = State::EscapeCommand;
                should_consume = false;
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
                return { Result::Escape, 0, sequence, {} };
            }

            case State::EscapeHash:
            {
                m_state = State::Ascii;

                auto sequence = EscapeSequence { '#', { c - '0' }, false };
                return { Result::Escape, 0, sequence, {} };
            }

            case State::EscapeBracket:
            {
                m_state = State::Ascii;

                auto sequence = EscapeSequence { '(', { c }, false };
                return { Result::Escape, 0, sequence, {} };
            }

            case State::OSCommandStart:
            {
                if (c == ';')
                {
                    m_state = State::OSCommandBody;
                    m_current_args.push_back(std::atoi(m_current_argument.c_str()));
                    m_current_argument.clear();
                    break;
                }

                m_current_argument += c;
                break;
            }

            case State::OSCommandBody:
            {
                if (c == 0x07)
                {
                    m_state = State::Ascii;
                    auto command = OSCommand { m_current_args[0], m_current_argument };
                    m_current_argument = "";
                    m_current_args.clear();
                    return { Result::OSCommand, 0, {}, command };
                }

                m_current_argument += c;
                break;
            }

            case State::Utf8OneByteLeft:
            {
                if ((c & 0b11000000) != 0b10000000)
                {
                    std::cout << "Invalid UTF-8\n";
                    m_state = State::Ascii;
                    should_consume = false;
                    break;
                }

                m_current_rune |= c & 0b00111111;
                m_state = State::Ascii;
                return { Result::Rune, m_current_rune, {}, {} };
            }

            case State::Utf8TwoBytesLeft:
            {
                if ((c & 0b11000000) != 0b10000000)
                {
                    std::cout << "Invalid UTF-8\n";
                    m_state = State::Ascii;
                    should_consume = false;
                    break;
                }

                m_current_rune |= (c & 0b00111111) << 6;
                m_state = State::Utf8OneByteLeft;
                break;
            }

            case State::Utf8ThreeBytesLeft:
            {
                if ((c & 0b11000000) != 0b10000000)
                {
                    std::cout << "Invalid UTF-8\n";
                    m_state = State::Ascii;
                    should_consume = false;
                    break;
                }

                m_current_rune |= (c & 0b00111111) << 12;
                m_state = State::Utf8TwoBytesLeft;
                break;
            }
        }
    } while (!should_consume);
    
    return { Result::Incomplete, 0, {}, {} };
}
