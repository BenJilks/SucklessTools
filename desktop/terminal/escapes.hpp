#pragma once
#include "color.hpp"
#include <string>
#include <memory>
#include <vector>
#include <optional>

namespace Escape
{

    class Attribute
    {
    public:
        Attribute() {}
        ~Attribute() = default;

        inline void add(TerminalColor::Type type, TerminalColor::Named color) 
        { 
            m_colors.push_back(std::make_pair(type, color));
        }
        inline void add(TerminalColor::Flags flag, bool enabled) 
        { 
            m_flags.push_back(std::make_pair(flag, enabled));
        }
        
        inline const auto &colors() const { return m_colors; }
        inline const auto &flags() const { return m_flags; }
        
    private:
        std::vector<std::pair<TerminalColor::Type, TerminalColor::Named>> m_colors;
        std::vector<std::pair<TerminalColor::Flags, bool>> m_flags;

    };

    class Cursor
    {
    public:
        enum Direction
        {
            Left,
            Right,
            Up,
            Down
        };
        
        Cursor(Direction direction)
            : m_direction(direction)
            , m_amount(1) {}

        Cursor(Direction direction, int amount)
            : m_direction(direction)
            , m_amount(amount) {}
            
        inline Direction direction() const { return m_direction; }
        inline int amount() const { return m_amount; }
        inline std::string name() const
        {
            switch (m_direction)
            {
                case Left: return "Left";
                case Right: return "Right";
                case Up: return "Up";
                case Down: return "Down";
            }
            
            return "Unkown";
        }
        
    private:
        Direction m_direction;
        int m_amount;

    };

    class SetMode
    {
    public:
        enum Mode
        {
            CursorVisable,
            CursorBlink,
            AutoWrap
        };
        
        SetMode(Mode mode, bool value)
            : m_mode(mode)
            , m_value(value) {}
        
        inline Mode mode() const { return m_mode; }
        inline bool value() const { return m_value; }
        
    private:
        Mode m_mode;
        bool m_value;
        
    };
    
    class SetCursor
    {
    public:
        SetCursor(int coloumn, int row) 
            : m_coloumn(coloumn)
            , m_row(row) {}
        
        inline int coloumn() const { return m_coloumn; }
        inline int row() const { return m_row; }
        
    private:
        int m_coloumn { 0 };
        int m_row { 0 };
        
    };

    class Insert
    {
    public:
        Insert(int count)
            : m_count(count) {}

        inline int count() const { return m_count; }
        
    private:
        int m_count;
        
    };

    class Delete
    {
    public:
        Delete(int count)
            : m_count(count) {}
        
        inline int count() const { return m_count; }
            
    private:
        int m_count;
        
    };

    class Clear
    {
    public:
        enum Mode
        {
            CursorToLineEnd,
            LineEndToCursor,
            Line,
            Screen
        };
        
        Clear(Mode mode)
            : m_mode(mode) {}

        inline Mode mode() const { return m_mode; }
        
    private:
        Mode m_mode;

    };

    class Bell
    {
    public:
        Bell() {}
    };

    class Sequence
    {
    public:
        enum class Type
        {
            Attribute,
            Cursor,
            SetMode,
            SetCursor,
            Insert,
            Delete,
            Clear,
            Bell
        };

        Sequence(Attribute v) 
            : m_type(Type::Attribute)
            , m_attribute(v) {}

        Sequence(Cursor v) 
            : m_type(Type::Cursor)
            , m_cursor(v) {}

        Sequence(SetMode v) 
            : m_type(Type::SetMode)
            , m_set_mode(v) {}
        
        Sequence(SetCursor v) 
            : m_type(Type::SetCursor)
            , m_set_cursor(v) {}
        
        Sequence(Insert v) 
            : m_type(Type::Insert)
            , m_insert(v) {}
            
        Sequence(Delete v) 
            : m_type(Type::Delete)
            , m_delete(v) {}
        
        Sequence(Clear v) 
            : m_type(Type::Clear)
            , m_clear(v) {}
        
        Sequence(Bell v) 
            : m_type(Type::Bell)
            , m_bell(v) {}
        
        Sequence(const Sequence &other);
        
        ~Sequence() {}
        
        inline Type type() const { return m_type; }
        inline const Attribute &attribute() const { return m_attribute; }
        inline const Cursor &cursor() const { return m_cursor; }
        inline const SetMode &set_mode() const { return m_set_mode; }
        inline const SetCursor &set_cursor() const { return m_set_cursor; }
        inline const Insert &insert() const { return m_insert; }
        inline const Delete &delete_() const { return m_delete; }
        inline const Clear &clear() const { return m_clear; }
        inline const Bell &bell() const { return m_bell; }
        
    private:
        Type m_type;
        union
        {
            Attribute m_attribute;
            Cursor m_cursor;
            SetMode m_set_mode;
            SetCursor m_set_cursor;
            Insert m_insert;
            Delete m_delete;
            Clear m_clear;
            Bell m_bell;
        };

    };
    
    std::optional<Sequence> interpret_sequence(char command, 
        std::vector<int> args, bool is_private);
    
}

std::ostream &operator<<(std::ostream &stream, const Escape::Sequence& sequence);
