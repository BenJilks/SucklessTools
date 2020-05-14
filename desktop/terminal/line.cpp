#include "line.hpp"
#include <iostream>

void Line::set(int coloumn, char c)
{
    while (coloumn >= m_data.length())
        m_data += ' ';
    m_data[coloumn] = c;
    
    m_is_dirty = true;
}

void Line::insert(int coloumn, char c)
{
    if (coloumn >= m_data.length())
        set(coloumn, c);
    
    m_data.insert(coloumn, std::string(&c, 1));
    m_is_dirty = true;
}

void Line::erase(int coloumn, int count)
{
    m_data.erase(coloumn, count);
    m_is_dirty = true;
}

void Line::set_attribute(int coloumn, TerminalColor::Type type, TerminalColor::Named color)
{
    const auto *current = curr_attribute(coloumn);
    
    TerminalColor attribute;
    if (current)
        attribute = *current;
    
    switch (type)
    {
        case TerminalColor::Foreground: attribute.set_foreground(color); break;
        case TerminalColor::Background: attribute.set_background(color); break;
        default: break;
    }
    
    m_attributes.push_back(std::make_pair(coloumn, attribute));
}

void Line::set_attribute(int coloumn, TerminalColor::Flags flag, bool enabled)
{
    const auto *current = curr_attribute(coloumn);
    
    TerminalColor attribute;
    if (current)
        attribute = *current;
    
    attribute.set_flag(flag, enabled);
    m_attributes.push_back(std::make_pair(coloumn, attribute));
}

void Line::set_attribute(int coloumn, TerminalColor color)
{
    const auto *current = curr_attribute(coloumn);
    
    TerminalColor attribute;
    if (current)
        attribute = *current;
    
    // Copy all the attributes over
    attribute.set_foreground(color.foreground());
    attribute.set_background(color.background());
    attribute.set_flag(TerminalColor::Bright, color.is(TerminalColor::Bright));
    attribute.set_flag(TerminalColor::Clear, color.is(TerminalColor::Clear));
    m_attributes.push_back(std::make_pair(coloumn, attribute));
}

const TerminalColor *Line::curr_attribute(int coloumn)
{
    int max = 0;
    const TerminalColor *max_attr = nullptr;
    for (const auto &it : m_attributes)
    {
        if (it.first <= coloumn && it.first >= max)
        {
            max = it.first;
            max_attr = &it.second;
        }
    }
    
    return max_attr;
}
