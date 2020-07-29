#pragma once
#include <string>

#include "config.hpp"

class TerminalColor
{
public:
    enum Named
    {
        DefaultBackground,
        DefaultForeground,
        
        Black, Red, Green, Yellow,
        Blue, Magenta, Cyan, White,
    };
    
    enum Type
    {
        Foreground,
        Background
    };
    
    enum Flags
    {
        Bright = 1 << 0,
        Clear = 1 << 1
    };

    bool operator== (const TerminalColor &other) const;
    bool operator!= (const TerminalColor &other) const { return !(*this == other); };
    
    TerminalColor(Named foreground, Named background, int flags = 0)
        : m_foreground(foreground)
        , m_background(background)
        , m_flags(flags) {}
    
    TerminalColor()
        : m_foreground(DefaultForeground)
        , m_background(DefaultBackground)
        , m_flags(0) {}
    
    inline static TerminalColor default_color() { return TerminalColor(DefaultForeground, DefaultBackground); }
    TerminalColor inverted() const;
    
    inline Named foreground() const { return m_foreground; }
    inline Named background() const { return m_background; }
    inline bool is(Flags flag) const { return m_flags & (int)flag; }
    inline void set_foreground(Named foreground) { m_foreground = foreground; }
    inline void set_background(Named background) { m_background = background; }
    inline void set_flag(Flags flag, bool enabled) 
    { 
        if (enabled) m_flags |= (int)flag; 
        else m_flags &= ~(int)flag;
    }

    int foreground_int() const;
    int background_int() const;
    std::string name() const;
    
private:
    Named m_foreground, m_background;
    int m_flags;

};
