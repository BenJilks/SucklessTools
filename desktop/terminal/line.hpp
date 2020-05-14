#pragma once
#include "color.hpp"
#include <string>
#include <vector>
#include <map>

class Line
{
public:
    Line() {}

    void set(int coloumn, char c);
    void insert(int coloumn, char c);
    void erase(int coloumn, int count);
    void set_attribute(int coloumn, TerminalColor::Type type, TerminalColor::Named color);
    void set_attribute(int coloumn, TerminalColor::Flags flag, bool enabled);
    void set_attribute(int coloumn, TerminalColor color);
    const TerminalColor *curr_attribute(int coloumn);
    
    inline bool is_dirty() const { return m_is_dirty; }
    inline void mark_dirty() { m_is_dirty = true; }
    inline void unmark_dirty() { m_is_dirty = false; }

    inline size_t length() const { return m_data.length(); }
    inline void clear() { return m_data.clear(); mark_dirty(); }
    
    inline bool was_in_selection() { return m_was_in_selection; }
    inline void mark_in_selection() { m_was_in_selection = true; }
    inline void unmark_in_selection() { m_was_in_selection = false; }
    inline const std::string &data() const { return m_data; }
    
private:
    std::string m_data;
    std::vector<std::pair<int, TerminalColor>> m_attributes;

    bool m_is_dirty { false };
    bool m_was_in_selection { false };
    
};
