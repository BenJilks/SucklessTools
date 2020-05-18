#include "escapes.hpp"
#include <optional>
#include <iostream>
#include <vector>
using namespace Escape;

enum class ColorTypeFlags
{
    Forground = 0x1 << 0,
    Background = 0x1 << 1,
    Bright = 0x1 << 2,
    Clear = 0x1 << 3,
};

Sequence::Sequence(const Sequence &other)
    : m_type(other.m_type)
{
    switch (other.m_type)
    {
        case Type::Attribute: m_attribute = other.m_attribute; break;
        case Type::Cursor: m_cursor = other.m_cursor; break;
        case Type::SetMode: m_set_mode = other.m_set_mode; break;
        case Type::SetCursor: m_set_cursor = other.m_set_cursor; break;
        case Type::Insert: m_insert = other.m_insert; break;
        case Type::Delete: m_delete = other.m_delete; break;
        case Type::Clear: m_clear = other.m_clear; break;
        case Type::Bell: m_bell = other.m_bell; break;
    }
}

static void decode_attribute_number(Attribute &attr, int num)
{
    if (num == 0)
    {
        attr.add(TerminalColor::Foreground, TerminalColor::DefaultForeground);
        attr.add(TerminalColor::Background, TerminalColor::DefaultBackground);
        attr.add(TerminalColor::Clear, true);
        attr.add(TerminalColor::Bright, false);
        return;
    }
    
    auto type = TerminalColor::Foreground;
    if (num >= 90)
    {
        attr.add(TerminalColor::Bright, true);
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
        case 0: attr.add(type, TerminalColor::Black); break;
        case 1: attr.add(type, TerminalColor::Red); break;
        case 2: attr.add(type, TerminalColor::Green); break;
        case 3: attr.add(type, TerminalColor::Yellow); break;
        case 4: attr.add(type, TerminalColor::Blue); break;
        case 5: attr.add(type, TerminalColor::Magenta); break;
        case 6: attr.add(type, TerminalColor::Cyan); break;
        case 7: attr.add(type, TerminalColor::White); break;
        default: break;
    }
}

std::optional<Sequence> Escape::interpret_sequence(
    char command, std::vector<int> args, bool is_private)
{
    auto argc = args.size();
    
    if (is_private)
    {
        if (command != 'h' && command != 'l')
            return std::nullopt;
        if (argc != 1)
            return std::nullopt;
        
        bool enabled = (command == 'h');
        switch (args[0])
        {
            case 7: return Escape::SetMode(SetMode::AutoWrap, enabled);
            case 12: return Escape::SetMode(SetMode::CursorBlink, enabled);
            case 25: return Escape::SetMode(SetMode::CursorVisable, enabled);
            default: return std::nullopt;
        }
    }
    else
    {
        switch (command)
        {
            case 'K': return Escape::Clear(Clear::CursorToLineEnd);
            case 'A': return Escape::Cursor(Cursor::Up, argc ? args[0] : 1);
            case 'B': return Escape::Cursor(Cursor::Down, argc ? args[0] : 1);
            case 'C': return Escape::Cursor(Cursor::Right, argc ? args[0] : 1);
            case 'D': return Escape::Cursor(Cursor::Left, argc ? args[0] : 1);
            case '@': return Escape::Insert(argc ? args[0] : 1);
            case 'P': return Escape::Delete(argc ? args[0] : 1);
            
            case 'H': 
            {
                if (argc > 2)
                    return std::nullopt;
                
                auto rows = argc >= 1 ? args[0] : 0;
                auto coloumns = argc >= 2 ? args[1] : 0;
                return Escape::SetCursor(coloumns, rows);
            }
            
            case 'J': 
            {
                if (argc <= 0)
                    return std::nullopt;
                
                Clear::Mode mode;
                switch (args[0])
                {
                    case 2: mode = Clear::Screen; break;
                    default: return std::nullopt;
                }

                return Escape::Clear(mode);
            }
            
            case 'm':
            {
                Escape::Attribute attribute;
                for (int num : args)
                    decode_attribute_number(attribute, num);
                return attribute;
            }
        }
    }
    
    return std::nullopt;
}

std::ostream &operator<<(std::ostream &stream, const Sequence& sequence)
{
    switch (sequence.type())
    {
        case Sequence::Type::Attribute: 
        {
            stream << "Attribute"; //(name = " << color.name() << ")";
            break;
        }

        case Sequence::Type::SetMode:
        {
            auto set_mode = sequence.set_mode();
            stream << "SetMode(mode = " << set_mode.mode() << ", value = " << set_mode.value() << ")";
            break;
        }

        case Sequence::Type::SetCursor:
        {
            auto set_cursor = sequence.set_cursor();
            stream << "SetCursor(coloumn = " << set_cursor.coloumn() << ", row = " << set_cursor.row() << ")";
            break;
        }
        
        case Sequence::Type::Cursor: 
        {
            auto cursor = sequence.cursor();
            stream << "Cursor(name = " << cursor.name() << ", amount = " << cursor.amount() << ")";
            break;
        }

        case Sequence::Type::Insert: 
        {
            auto insert = sequence.insert();
            stream << "Insert(count = " << insert.count() << ")";
            break;
        }

        case Sequence::Type::Delete: 
        {
            auto del = sequence.delete_();
            stream << "Delete(count = " << del.count() << ")";
            break;
        }

        case Sequence::Type::Clear: 
        {
            stream << "Clear";
            break;
        }

        case Sequence::Type::Bell:
            stream << "Bell";
            break;
    }
    
    return stream;
}
