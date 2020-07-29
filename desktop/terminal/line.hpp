#pragma once
#include "color.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdint.h>

struct Rune
{
    uint32_t value;
    TerminalColor color;
};

/*
class Line
{
public:
    Line(Line&) = delete;
    Line(const Line&) = delete;

    Line(Rune *data, int columns)
        : m_data(data)
        , m_columns(columns) {}

    // Iterator setup methods
    const auto begin() const { return m_data.begin(); }
    const auto end() const { return m_data.end(); }
    const Rune &operator[](int index);
    const Rune &first() const;
    const Rune &last() const;
    
    void set(int column, uint32_t c);
    void insert(int column, uint32_t c);
    void erase(int column, int count);
    void set_attribute(int column, TerminalColor::Type type, TerminalColor::Named color);
    void set_attribute(int column, TerminalColor::Flags flag, bool enabled);
    void set_attribute(int column, TerminalColor color);
    
    inline bool is_dirty() const { return m_is_dirty; }
    inline void mark_dirty() { m_is_dirty = true; }
    inline void unmark_dirty() { m_is_dirty = false; }
    inline void clear();
    
    inline bool was_in_selection() { return m_was_in_selection; }
    inline void mark_in_selection() { m_was_in_selection = true; }
    inline void unmark_in_selection() { m_was_in_selection = false; }
    
private:
    void modify_attribute(int column, std::function<void(TerminalColor&)> callback);
    
    Rune *m_data;
    int m_columns;

    bool m_is_dirty { false };
    bool m_was_in_selection { false };
    
};
*/
