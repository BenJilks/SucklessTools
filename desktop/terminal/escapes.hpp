#pragma once
#include "color.hpp"
#include <string>
#include <memory>

namespace Escape
{

    class Sequence
    {
    public:
        virtual ~Sequence() {}

        static std::unique_ptr<Sequence> parse(std::string_view str);
        
        enum Type
        {
            Color,
            Cursor,
            Insert,
            Delete,
            Clear,
            Bell
        };
        
        inline Type type() const { return m_type; }
        inline int char_count() const { return m_char_count; }

    protected:
        Sequence(Type type, int char_count) 
            : m_type(type)
            , m_char_count(char_count) {}
        
    private:
        Type m_type;
        int m_char_count;

    };

    class Color : public Sequence
    {
    public:
        Color(TerminalColor color, int char_count)
            : Sequence(Sequence::Color, char_count)
            , m_color(color) {}

        virtual ~Color() {}

        inline std::string name() { return m_color.name(); }
        inline const TerminalColor &color() { return m_color; }
        
    private:
        TerminalColor m_color;

    };

    class Cursor : public Sequence
    {
    public:
        enum Direction
        {
            Left,
            Right,
            Up,
            Down,
            TopLeft
        };
        
        Cursor(Direction direction, int char_count)
            : Sequence(Sequence::Cursor, char_count)
            , m_direction(direction) {}
        
        inline Direction direction() const { return m_direction; }
        inline std::string name() const
        {
            switch (m_direction)
            {
                case Left: return "Left";
                case Right: return "Right";
                case Up: return "Up";
                case Down: return "Down";
                case TopLeft: return "TopLeft";
            }
            
            return "Unkown";
        }
        
    private:
        Direction m_direction;

    };

    class Insert : public Sequence
    {
    public:
        Insert(int count, int char_count)
            : Sequence(Sequence::Insert, char_count)
            , m_count(count) {}

        inline int count() const { return m_count; }
        
    private:
        int m_count;
        
    };

    class Delete : public Sequence
    {
    public:
        Delete(int count, int char_count)
            : Sequence(Sequence::Delete, char_count)
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
        
        Clear(Mode mode, int char_count)
            : Sequence(Sequence::Clear, char_count)
            , m_mode(mode) {}

        inline Mode mode() const { return m_mode; }
        
    private:
        Mode m_mode;

    };

    class Bell : public Sequence
    {
    public:
        Bell()
            : Sequence(Sequence::Bell, 0) {}
    };

}

std::ostream &operator<<(std::ostream &stream, const Escape::Sequence& sequence);
