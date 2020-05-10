#pragma once
#include <string>
#include <memory>

class EscapesSequence
{
public:
    virtual ~EscapesSequence() {}

    static std::unique_ptr<EscapesSequence> parse(std::string_view str);
    
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
    EscapesSequence(Type type, int char_count) 
        : m_type(type)
        , m_char_count(char_count) {}
    
private:
    Type m_type;
    int m_char_count;

};

std::ostream &operator<<(std::ostream &stream, const EscapesSequence& sequence);

class EscapeColor : public EscapesSequence
{
public:
    enum Color
    {
        Black, Red, Green, Yellow,
        Blue, Magenta, Cyan, White
    };
    
    enum Flags
    {
        Bright = 1 << 0,
        Clear = 1 << 1
    };
    
    EscapeColor(Color color, int char_count, int flags = 0)
        : EscapesSequence(EscapesSequence::Color, char_count)
        , m_color(color)
        , m_flags(flags) {}

    virtual ~EscapeColor() {}
    
    inline Color id() const { return m_color; }
    inline bool is_bright() const { return m_flags & Bright; }
    inline bool is_clear() const { return m_flags & Clear; }

    inline std::string name() const
    {
        switch (m_color)
        {
            case Black: return "Black";
            case Red: return "Red";
            case Green: return "Green";
            case Yellow: return "Yellow";
            case Blue: return "Blue";
            case Magenta: return "Magenta";
            case Cyan: return "Cyan";
            case White: return "White";
        }
        
        return "Unkown(" + std::to_string(m_color) + ")";
    }
    
private:
    Color m_color;
    int m_flags;

};

class EscapeCursor : public EscapesSequence
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
    
    EscapeCursor(Direction direction, int char_count)
        : EscapesSequence(Cursor, char_count)
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

class EscapeInsert : public EscapesSequence
{
public:
    EscapeInsert(int count, int char_count)
        : EscapesSequence(Insert, char_count)
        , m_count(count) {}

    inline int count() const { return m_count; }
    
private:
    int m_count;
    
};

class EscapeDelete : public EscapesSequence
{
public:
    EscapeDelete(int count, int char_count)
        : EscapesSequence(Delete, char_count)
        , m_count(count) {}
    
    inline int count() const { return m_count; }
        
private:
    int m_count;
    
};

class EscapeClear : public EscapesSequence
{
public:
    enum Mode
    {
        CursorToLineEnd,
        LineEndToCursor,
        Line,
        Screen
    };
    
    EscapeClear(Mode mode, int char_count)
        : EscapesSequence(Clear, char_count)
        , m_mode(mode) {}

    inline Mode mode() const { return m_mode; }
    
private:
    Mode m_mode;

};

class EscapeBell : public EscapesSequence
{
public:
    EscapeBell()
        : EscapesSequence(Bell, 0) {}
};
