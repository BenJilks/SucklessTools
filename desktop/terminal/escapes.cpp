#include "escapes.hpp"
#include <optional>
#include <iostream>
#include <vector>
using namespace Escape;

static std::optional<int> parse_number(std::string_view str, int &index)
{
    std::string num;
    while (index < str.length())
    {
        char c = str[index];
        if (!std::isdigit(c))
            break;
        
        num += c;
        index += 1;
    }

    if (num.length() == 0)
        return std::nullopt;
    
    return std::atoi(num.c_str());
}

enum class ColorTypeFlags
{
    Forground = 0x1 << 0,
    Background = 0x1 << 1,
    Bright = 0x1 << 2,
    Clear = 0x1 << 3,
};

static void decode_attribute_number(std::unique_ptr<Attribute> &attr, int num)
{
    if (num == 0)
    {
        attr->add(TerminalColor::Foreground, TerminalColor::DefaultForeground);
        attr->add(TerminalColor::Background, TerminalColor::DefaultBackground);
        attr->add(TerminalColor::Clear, true);
        attr->add(TerminalColor::Bright, false);
        return;
    }
    
    auto type = TerminalColor::Foreground;
    if (num >= 90)
    {
        attr->add(TerminalColor::Bright, true);
        num -= 60;
    }
    
    num -= 30;
    if (num >= 10)
    {
        type = TerminalColor::Background;
        num -= 10;
    }
    
    switch (num)
    {
        case 0: attr->add(type, TerminalColor::Black); break;
        case 1: attr->add(type, TerminalColor::Red); break;
        case 2: attr->add(type, TerminalColor::Green); break;
        case 3: attr->add(type, TerminalColor::Yellow); break;
        case 4: attr->add(type, TerminalColor::Blue); break;
        case 5: attr->add(type, TerminalColor::Magenta); break;
        case 6: attr->add(type, TerminalColor::Cyan); break;
        case 7: attr->add(type, TerminalColor::White); break;
        default: break;
    }
}

static std::unique_ptr<Sequence> parse_private(std::string_view str, int &index)
{
    index += 1;
    auto num = parse_number(str, index);
    if (!num)
        return nullptr;
    
    switch (*num)
    {
        case 7:
            switch (str[index])
            {
                case 'h': return std::make_unique<Escape::Cursor>(Escape::Cursor::EnableAutoWrap, index);
                case 'l': return std::make_unique<Escape::Cursor>(Escape::Cursor::DisableAutoWrap, index);
                default: break;
            }
            break;
        
        case 25:
            switch (str[index])
            {
                case 'h': return std::make_unique<Escape::Cursor>(Escape::Cursor::Show, index);
                case 'l': return std::make_unique<Escape::Cursor>(Escape::Cursor::Hide, index);
                default: break;
            }
            break;
        
        default:
            std::cout << "Unkown mode " << *num << " = " << 
                (str[index] == 'h' ? "enable" : "disable") << "\n";
            break;
    }
    
    return nullptr;
}

std::unique_ptr<Sequence> Sequence::parse(std::string_view str)
{
    if (str.length() < 3 || (str[0] != '\033' && str[1] != '['))
    {
        if (str.length() >= 1)
        {
            switch (str[0])
            {
                case 7: return std::make_unique<Escape::Bell>();
                case 8: return std::make_unique<Escape::Cursor>(Escape::Cursor::Left, 0);
                default: break;
            }
        }
        return nullptr;
    }
    
    int index = 2;
    auto first_num = parse_number(str, index);
    if (!first_num)
    {
        if (index >= str.length())
            return nullptr;
        
        switch (str[index])
        {
            case 'K': return std::make_unique<Escape::Clear>(Escape::Clear::CursorToLineEnd, 2);
            case 'H': return std::make_unique<Escape::Cursor>(Escape::Cursor::TopLeft, 2);
            case 'A': return std::make_unique<Escape::Cursor>(Escape::Cursor::Up, 2);
            case 'B': return std::make_unique<Escape::Cursor>(Escape::Cursor::Down, 2);
            case 'C': return std::make_unique<Escape::Cursor>(Escape::Cursor::Left, 2);
            case 'D': return std::make_unique<Escape::Cursor>(Escape::Cursor::Right, 2);
            case '?':
            {
                auto private_escape = parse_private(str, index);
                if (private_escape)
                    return private_escape;
                
                break;
            }
            default: break;
        }
        return nullptr;
    }
    
    if (index < str.length())
    {
        char c = str[index];
        switch (c)
        {
            case 'm': 
            {
                auto attr = std::make_unique<Escape::Attribute>();
                decode_attribute_number(attr, *first_num);
                attr->set_char_count(index);
                return std::move(attr);
            }
            
            case ';':
            {
                auto attr = std::make_unique<Escape::Attribute>();
                decode_attribute_number(attr, *first_num);
                
                while (str[index] == ';')
                {
                    auto num = parse_number(str, ++index);
                    if (!num || index > str.length())
                        break;
                    decode_attribute_number(attr, *num);
                }
                
                if (str[index] != 'm')
                    return nullptr;
                
                attr->set_char_count(index);
                return std::move(attr);
            }
            
            case '@':
            {
                return std::make_unique<Escape::Insert>(
                    *first_num, index);
            }
            
            case 'P':
            {
                return std::make_unique<Escape::Delete>(
                    *first_num, index);
            }
            
            case 'J':
            {
                switch (*first_num)
                {
                    case 2: 
                        return std::make_unique<Escape::Clear>(
                            Escape::Clear::Screen, index);
                        break;
                        
                    default: 
                        break;
                }
            }
            
            case 'A':
            {
                return std::make_unique<Escape::Cursor>(
                    Escape::Cursor::Up, *first_num, index);
            }

            case 'B':
            {
                return std::make_unique<Escape::Cursor>(
                    Escape::Cursor::Down, *first_num, index);
            }

            case 'C':
            {
                return std::make_unique<Escape::Cursor>(
                    Escape::Cursor::Right, *first_num, index);
            }

            case 'D':
            {
                return std::make_unique<Escape::Cursor>(
                    Escape::Cursor::Left, *first_num, index);
            }

            default:
                break;
        }
    }
    
    std::cout << "FIXME: escapes.cpp: parse: Add single number parsing here\n";
    return nullptr;
}

std::ostream &operator<<(std::ostream &stream, const Sequence& sequence)
{
    switch (sequence.type())
    {
        case Sequence::Attribute: 
        {
            auto color = static_cast<const Attribute&>(sequence);
            stream << "Attribute"; //(name = " << color.name() << ")";
            break;
        }

        case Sequence::Cursor: 
        {
            auto cursor = static_cast<const Cursor&>(sequence);
            stream << "Cursor(name = " << cursor.name() << ", amount = " << cursor.amount() << ")";
            break;
        }

        case Sequence::Insert: 
        {
            auto insert = static_cast<const Insert&>(sequence);
            stream << "Insert(count = " << insert.count() << ")";
            break;
        }

        case Sequence::Delete: 
        {
            auto del = static_cast<const Insert&>(sequence);
            stream << "Delete(count = " << del.count() << ")";
            break;
        }

        case Sequence::Clear: 
        {
            auto clear = static_cast<const Clear&>(sequence);
            stream << "Clear";
            break;
        }

        case Sequence::Bell:
            stream << "Bell";
            break;
    }
    
    return stream;
}
