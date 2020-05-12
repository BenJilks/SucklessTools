#pragma once
#include <string>

namespace ColorPalette
{
    static std::string Black = "000000";
    static std::string Red = "FF0000";
    static std::string Green = "00FF00";
    static std::string Yellow = "FFFF00";
    static std::string Blue = "0000FF";
    static std::string Magenta = "FF00FF";
    static std::string Cyan = "00FFFF";
    static std::string White = "FFFFFF";
};

class TerminalColor
{
public:
    enum Named
    {
        Black, Red, Green, Yellow,
        Blue, Magenta, Cyan, White,
    };
    
    enum Flags
    {
        Bright = 1 << 0,
        Clear = 1 << 1
    };

    TerminalColor(Named foreground, Named background, int flags = 0)
        : m_foreground(foreground)
        , m_background(background)
        , m_flags(flags) {}
    
    TerminalColor()
        : m_foreground(White)
        , m_background(Black)
        , m_flags(0) {}
    
    TerminalColor inverted() const;
    
    inline Named foreground() const { return m_foreground; }
    inline Named background() const { return m_background; }
    inline bool is(Flags flag) const { return m_flags & (int)flag; }
    inline void set_foreground(Named foreground) { m_foreground = foreground; }
    inline void set_background(Named background) { m_background = background; }
    inline void set_flag(Flags flag) { m_flags |= (int)flag; }
    int foreground_int() const;
    int background_int() const;
    std::string name() const;

private:
    Named m_foreground, m_background;
    int m_flags;

};

