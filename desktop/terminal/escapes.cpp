#include "escapes.hpp"
#include <optional>
#include <iostream>

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

std::unique_ptr<EscapesSequence> EscapesSequence::parse(std::string_view str)
{
    if (str.length() < 3 || (str[0] != '\033' && str[1] != '['))
    {
        if (str.length() >= 1)
        {
            switch (str[0])
            {
                case 7: return std::make_unique<EscapeBell>();
                case 8: return std::make_unique<EscapeCursor>(EscapeCursor::Right, 0);
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
            case 'K': return std::make_unique<EscapeClear>(EscapeClear::CursorToLineEnd, 2);
            case 'H': return std::make_unique<EscapeCursor>(EscapeCursor::TopLeft, 2);
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
                return std::make_unique<EscapeColor>(
                    EscapeColor::Black, index);
            
            case ';':
            {
                auto second_num = parse_number(str, ++index);
                if (!second_num || index > str.length())
                    break;
                
                if (str[index] != 'm')
                    return nullptr;
                
                return std::make_unique<EscapeColor>(
                    EscapeColor::Black, index);
            }
            
            case '@':
            {
                return std::make_unique<EscapeInsert>(
                    *first_num, index);
            }
            
            case 'P':
            {
                return std::make_unique<EscapeDelete>(
                    *first_num, index);
            }
            
            case 'J':
            {
                switch (*first_num)
                {
                    case 2: 
                        return std::make_unique<EscapeClear>(
                            EscapeClear::Screen, index);
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

std::ostream &operator<<(std::ostream &stream, const EscapesSequence& sequence)
{
    switch (sequence.type())
    {
        case EscapesSequence::Color: 
        {
            auto color = static_cast<const EscapeColor&>(sequence);
            stream << "Color(name = " << color.name() << ")";
            break;
        }

        case EscapesSequence::Cursor: 
        {
            auto cursor = static_cast<const EscapeCursor&>(sequence);
            stream << "Cursor(name = " << cursor.name() << ")";
            break;
        }

        case EscapesSequence::Insert: 
        {
            auto insert = static_cast<const EscapeInsert&>(sequence);
            stream << "Insert(count = " << insert.count() << ")";
            break;
        }

        case EscapesSequence::Delete: 
        {
            auto del = static_cast<const EscapeInsert&>(sequence);
            stream << "Delete(count = " << del.count() << ")";
            break;
        }

        case EscapesSequence::Clear: 
        {
            auto clear = static_cast<const EscapeClear&>(sequence);
            stream << "Clear";
            break;
        }

        case EscapesSequence::Bell:
            stream << "Bell";
            break;
    }
    
    return stream;
}
