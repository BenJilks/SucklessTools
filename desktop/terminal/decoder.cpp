#include "decoder.hpp"

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
                        return
                        {
                            Result::Escape,
                            std::make_unique<Escape::Bell>()
                        };
                        break;

                    case 8:
                        return
                        {
                            Result::Escape,
                            std::make_unique<Escape::Cursor>(
                                Escape::Cursor::Left, 1)
                        };
                        break;
                    
                    default:
                        return { Result::Rune, c };
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
                
                auto sequence = Escape::Sequence::interpret_sequence(
                    c, m_current_args, 
                    m_current_is_private);
                if (!sequence)
                    break;
                
                return { Result::Escape, std::move(sequence) };
            }
        }
    } while (!should_consume);
    
    return { Result::Incomplete };
}
