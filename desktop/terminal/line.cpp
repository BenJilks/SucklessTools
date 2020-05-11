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

void Line::set_attribute(int coloumn, Attribute attr)
{
    m_attributes.push_back(std::make_pair(coloumn, attr));
}

const Attribute *Line::curr_attribute(int coloumn)
{
    int max = 0;
    const Attribute *max_attr = nullptr;
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
