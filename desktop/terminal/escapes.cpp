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

void decode_color_name(int num, TerminalColor &color)
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

TerminalColor parse_color(std::vector<int> color_nums)
{
    TerminalColor color;
    
    for (int num : color_nums)
        decode_color_name(num, color);
    return color;
}

std::unique_ptr<Sequence> parse_private(std::string_view str, int &index)
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
                if (*first_num == 0)
                {
                    return std::make_unique<Escape::Color>(TerminalColor(
                        TerminalColor::White, TerminalColor::Black, 
                        TerminalColor::Clear), index);
                }
                
                return std::make_unique<Escape::Color>(
                    parse_color(std::vector<int> { *first_num }), index);
            }
            
            case ';':
            {
                std::vector<int> color_nums;
                color_nums.push_back(*first_num);
                
                while (str[index] == ';')
                {
                    auto num = parse_number(str, ++index);
                    if (!num || index > str.length())
                        break;
                    color_nums.push_back(*num);
                }
                
                if (str[index] != 'm')
                    return nullptr;
                
                return std::make_unique<Escape::Color>(
                    parse_color(color_nums), index);
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
        case Sequence::Color: 
        {
            auto color = static_cast<const Color&>(sequence);
            stream << "Color(name = " << color.name() << ")";
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
