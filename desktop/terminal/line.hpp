#pragma once
#include "color.hpp"
#include <string>
#include <vector>

class Attribute
{
public:
    Attribute(TerminalColor color)
        : m_color(color) {}

    inline TerminalColor color() const { return m_color; }

private:
    TerminalColor m_color;

};

class Line
{
public:
    Line() {}

    void set(int coloumn, char c);
    void insert(int coloumn, char c);
    void erase(int coloumn, int count);
    void set_attribute(int coloumn, Attribute attr);
    const Attribute *curr_attribute(int coloumn);
    
    inline bool is_dirty() const { return m_is_dirty; }
    inline void mark_dirty() { m_is_dirty = true; }
    inline void unmark_dirty() { m_is_dirty = false; }

    inline bool was_in_selection() { return m_was_in_selection; }
    inline void mark_in_selection() { m_was_in_selection = true; }
    inline void unmark_in_selection() { m_was_in_selection = false; }
    inline const std::string &data() const { return m_data; }
        
private:
    std::string m_data;
    std::vector<std::pair<int, Attribute>> m_attributes;
    bool m_is_dirty { false };
    bool m_was_in_selection { false };
    
};
