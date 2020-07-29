#include "line.hpp"
#include <iostream>

/*
const Rune &Line::operator[](int index)
{
    if (index >= m_data.size())
        set(index, ' ');
    return m_data[index];
}

const Rune &Line::first() const
{
    static Rune blank = { ' ' };

    if (m_data.size() <= 0)
        return blank;
    
    return m_data[0];
}

const Rune &Line::last() const
{
    static Rune blank = { ' ' };

    if (m_data.size() <= 0)
        return blank;
    
    return m_data.back();
}

void Line::set(int column, uint32_t c)
{
    while (column >= m_data.size())
    {
        TerminalColor attr;
        if (m_data.size() > 0)
            attr = m_data[m_data.size()-1].color;
        m_data.push_back({ ' ', attr });
    }
    m_data[column].value = c;
    
    m_is_dirty = true;
}

void Line::insert(int column, uint32_t c)
{
    if (column >= m_data.size())
    {
        set(column, c);
        return;
    }
    
    auto attr = m_data[column].color;
    m_data.insert(m_data.begin() + column, { c, attr });
    m_is_dirty = true;
}

void Line::erase(int column, int count)
{
    m_data.erase(m_data.begin() + column, m_data.begin() + column + count);
    m_is_dirty = true;
}

void Line::modify_attribute(int column, std::function<void(TerminalColor&)> callback)
{
    if (column >= m_data.size())
        set(column, ' ');
    
    auto attr = m_data[column].color;
    auto original = attr;
    callback(attr);

    m_data[column].color = attr;
    for (int i = column; i < m_data.size(); i++)
    {
        // Cascade this change down the rest of the line
        if (m_data[i].color == original)
            m_data[i].color = attr;
    }
}

void Line::set_attribute(int column, TerminalColor::Type type, TerminalColor::Named color)
{
    modify_attribute(column, [&](TerminalColor &attr)
    {
        switch (type)
        {
            case TerminalColor::Foreground: attr.set_foreground(color); break;
            case TerminalColor::Background: attr.set_background(color); break;
            default: break;
        }
    });
}

void Line::set_attribute(int column, TerminalColor::Flags flag, bool enabled)
{
    modify_attribute(column, [&](TerminalColor &attr)
    {
        attr.set_flag(flag, enabled);
    });
}

void Line::set_attribute(int column, TerminalColor color)
{
    modify_attribute(column, [&](TerminalColor &attr)
    {
        // Copy all the attributes over
        attr.set_foreground(color.foreground());
        attr.set_background(color.background());
        attr.set_flag(TerminalColor::Bright, color.is(TerminalColor::Bright));
        attr.set_flag(TerminalColor::Clear, color.is(TerminalColor::Clear));
    });
}
*/
