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

std::unique_ptr<Sequence> Sequence::interpret_sequence(
    char command, std::vector<int> args, bool is_private)
{
    auto argc = args.size();
    
    if (is_private)
    {
        if (command != 'h' && command != 'l')
            return nullptr;
        if (argc != 1)
            return nullptr;
        
        bool enabled = (command == 'h');
        switch (args[0])
        {
            case 7: 
                return std::make_unique<Escape::SetMode>(
                    SetMode::AutoWrap, enabled);
            case 12: 
                return std::make_unique<Escape::SetMode>(
                    SetMode::CursorBlink, enabled);
            case 25: 
                return std::make_unique<Escape::SetMode>(
                    SetMode::CursorVisable, enabled);
            default: return nullptr;
        }
    }
    else
    {
        switch (command)
        {
            case 'K': 
                return std::make_unique<Escape::Clear>(
                    Clear::CursorToLineEnd);
            case 'A': 
                return std::make_unique<Escape::Cursor>(
                    Cursor::Up, argc ? args[0] : 1);
            case 'B': 
                return std::make_unique<Escape::Cursor>(
                    Cursor::Down, argc ? args[0] : 1);
            case 'C': 
                return std::make_unique<Escape::Cursor>(
                    Cursor::Right, argc ? args[0] : 1);
            case 'D': 
                return std::make_unique<Escape::Cursor>(
                    Cursor::Left, argc ? args[0] : 1);
            case '@': 
                return std::make_unique<Escape::Insert>(
                    argc ? args[0] : 1);
            case 'P': 
                return std::make_unique<Escape::Delete>(
                    argc ? args[0] : 1);
            
            case 'H': 
            {
                if (argc > 2)
                    return nullptr;
                
                auto rows = argc >= 1 ? args[0] : 0;
                auto coloumns = argc >= 2 ? args[1] : 0;
                return std::make_unique<Escape::SetCursor>(
                    coloumns, rows);
            }
            
            case 'J': 
            {
                if (argc <= 0)
                    return nullptr;
                
                Clear::Mode mode;
                switch (args[0])
                {
                    case 2: mode = Clear::Screen; break;
                    default: return nullptr;
                }

                return std::make_unique<Escape::Clear>(mode);
            }
            
            case 'm':
            {
                auto attribute = std::make_unique<Escape::Attribute>();
                for (int num : args)
                    decode_attribute_number(attribute, num);
                return std::move(attribute);
            }
        }
    }
    
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

        case Sequence::SetMode:
        {
            auto set_mode = static_cast<const SetMode&>(sequence);
            stream << "SetMode(mode = " << set_mode.mode() << ", value = " << set_mode.value() << ")";
            break;
        }

        case Sequence::SetCursor:
        {
            auto set_cursor = static_cast<const SetCursor&>(sequence);
            stream << "SetCursor(coloumn = " << set_cursor.coloumn() << ", row = " << set_cursor.row() << ")";
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
