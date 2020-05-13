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

        static std::unique_ptr<Sequence> parse(std::string_view str);
        
        enum Type
        {
            Attribute,
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
            : m_char_count(char_count)
            , m_type(type) {}
        
        int m_char_count;
        
    private:
        Type m_type;

    };

    class Attribute : public Sequence
    {
    public:
        Attribute()
            : Sequence(Sequence::Attribute, 0) {}

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
        inline void set_char_count(int count) { m_char_count = count; }
        
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
            Down,

            TopLeft,
            Hide,
            Show,
            EnableAutoWrap,
            DisableAutoWrap
        };
        
        Cursor(Direction direction, int char_count)
            : Sequence(Sequence::Cursor, char_count)
            , m_direction(direction)
            , m_amount(1) {}

        Cursor(Direction direction, int amount, int char_count)
            : Sequence(Sequence::Cursor, char_count)
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
                
                case TopLeft: return "TopLeft";
                case Hide: return "Hide";
                case Show: return "Show";
                case EnableAutoWrap: return "EnableAutoWrap";
                case DisableAutoWrap: return "DisableAutoWrap";
            }
            
            return "Unkown";
        }
        
    private:
        Direction m_direction;
        int m_amount;

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
