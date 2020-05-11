#include "escapes.hpp"
#include <optional>
#include <iostream>
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

void parser_color_name(int num, TerminalColor &color)
{
    enum Type
    {
        Foregroud,
        Background
    };
    
    if (num == 0)
    {
        color.set_flag(TerminalColor::Clear);
        return;
    }
    
    auto type = Foregroud;
    if (num >= 90)
    {
        color.set_flag(TerminalColor::Bright);
        num -= 60;
    }
    
    num -= 30;
    if (num >= 10)
    {
        type = Background;
        num -= 10;
    }
    
    auto name = TerminalColor::White;
    switch (num)
    {
        case 0: name = TerminalColor::Black; break;
        case 1: name = TerminalColor::Red; break;
        case 2: name = TerminalColor::Green; break;
        case 3: name = TerminalColor::Yellow; break;
        case 4: name = TerminalColor::Blue; break;
        case 5: name = TerminalColor::Magenta; break;
        case 6: name = TerminalColor::Cyan; break;
        case 7: name = TerminalColor::White; break;
        default: break;
    }
    
    switch (type)
    {
       case Foregroud: color.set_foreground(name); break;
       case Background: color.set_background(name); break;
    }
}

TerminalColor parse_color(int first_num, int last_num = -1)
{
    TerminalColor color;
    parser_color_name(first_num, color);
    if (last_num != -1)
        parser_color_name(last_num, color);
    return color;
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
                case 8: return std::make_unique<Escape::Cursor>(Escape::Cursor::Right, 0);
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
                if (*first_num == 0)
                {
                    return std::make_unique<Escape::Color>(TerminalColor(
                        TerminalColor::White, TerminalColor::Black, 
                        TerminalColor::Clear), index);
                }
                
                return std::make_unique<Escape::Color>(
                    parse_color(*first_num), index);
            }
            
            case ';':
            {
                auto second_num = parse_number(str, ++index);
                if (!second_num || index > str.length())
                    break;
                
                if (str[index] != 'm')
                    return nullptr;
                
                return std::make_unique<Escape::Color>(
                    parse_color(*first_num, *second_num), index);
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
        case Sequence::Color: 
        {
            auto color = static_cast<const Color&>(sequence);
            stream << "Color(name = " << color.name() << ")";
            break;
        }

        case Sequence::Cursor: 
        {
            auto cursor = static_cast<const Cursor&>(sequence);
            stream << "Cursor(name = " << cursor.name() << ")";
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
