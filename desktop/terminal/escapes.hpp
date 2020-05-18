#pragma once
#include "color.hpp"
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <functional>

namespace Escape
{

    class Attribute
    {
    public:
        Attribute() {}
        ~Attribute() = default;

        inline void add(TerminalColor::Type type, TerminalColor::Named color) 
        { 
            switch (type)
            {
                case TerminalColor::Foreground: m_foreground = color; break;
                case TerminalColor::Background: m_background = color; break;
                default: break;
            }
        }
        inline void add(TerminalColor::Flags flag, bool enabled) 
        {
            switch (flag)
            {
                case TerminalColor::Bright: m_flag_bright = enabled; break;
                case TerminalColor::Clear: m_flag_clear = enabled; break;
                default: break;
            }
        }
        
        inline void for_each_color(std::function<void(TerminalColor::Type, TerminalColor::Named)> callback) const
        {
            if (m_foreground) callback(TerminalColor::Foreground, *m_foreground);
            if (m_background) callback(TerminalColor::Background, *m_background);
        }

        inline void for_each_flag(std::function<void(TerminalColor::Flags, bool)> callback) const
        {
            if (m_flag_bright) callback(TerminalColor::Bright, *m_flag_bright);
            if (m_flag_clear) callback(TerminalColor::Clear, *m_flag_clear);
        }
        
    private:
        std::optional<TerminalColor::Named> m_foreground { std::nullopt };
        std::optional<TerminalColor::Named> m_background { std::nullopt };
        std::optional<bool> m_flag_bright { std::nullopt };
        std::optional<bool> m_flag_clear { std::nullopt };

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
