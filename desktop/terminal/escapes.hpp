#pragma once
#include "color.hpp"
#include <string>
#include <memory>
#include <vector>

namespace Escape
{

    class Sequence
    {
    public:
        virtual ~Sequence() {}

        static std::unique_ptr<Sequence> interpret_sequence(
            char command, std::vector<int> args, bool is_private);
        
        enum Type
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
        
        inline Type type() const { return m_type; }

    protected:
        Sequence(Type type) 
            : m_type(type) {}
        
    private:
        Type m_type;

    };

    class Attribute : public Sequence
    {
    public:
        Attribute()
            : Sequence(Sequence::Attribute) {}

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
        
        virtual ~Attribute() {}
        
    private:
        std::vector<std::pair<TerminalColor::Type, TerminalColor::Named>> m_colors;
        std::vector<std::pair<TerminalColor::Flags, bool>> m_flags;

    };

    class Cursor : public Sequence
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
            : Sequence(Sequence::Cursor)
            , m_direction(direction)
            , m_amount(1) {}

        Cursor(Direction direction, int amount)
            : Sequence(Sequence::Cursor)
            , m_direction(direction)
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

    class SetMode : public Sequence
    {
    public:
        enum Mode
        {
            CursorVisable,
            CursorBlink,
            AutoWrap
        };
        
        SetMode(Mode mode, bool value)
            : Sequence(Sequence::SetMode)
            , m_mode(mode)
            , m_value(value) {}
        
        inline Mode mode() const { return m_mode; }
        inline bool value() const { return m_value; }
        
    private:
        Mode m_mode;
        bool m_value;
        
    };
    
    class SetCursor : public Sequence
    {
    public:
        SetCursor(int coloumn, int row) 
            : Sequence(Sequence::SetCursor)
            , m_coloumn(coloumn)
            , m_row(row) {}
        
        inline int coloumn() const { return m_coloumn; }
        inline int row() const { return m_row; }
        
    private:
        int m_coloumn { 0 };
        int m_row { 0 };
        
    };

    class Insert : public Sequence
    {
    public:
        Insert(int count)
            : Sequence(Sequence::Insert)
            , m_count(count) {}

        inline int count() const { return m_count; }
        
    private:
        int m_count;
        
    };

    class Delete : public Sequence
    {
    public:
        Delete(int count)
            : Sequence(Sequence::Delete)
            , m_count(count) {}
        
        inline int count() const { return m_count; }
            
    private:
        int m_count;
        
    };

    class Clear : public Sequence
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
            : Sequence(Sequence::Clear)
            , m_mode(mode) {}

        inline Mode mode() const { return m_mode; }
        
    private:
        Mode m_mode;

    };

    class Bell : public Sequence
    {
    public:
        Bell()
            : Sequence(Sequence::Bell) {}
    };

}

std::ostream &operator<<(std::ostream &stream, const Escape::Sequence& sequence);
