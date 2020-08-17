#pragma once
#include <algorithm>

class CursorPosition
{
public:
    CursorPosition(int coloumn, int row)
        : m_coloumn(coloumn), m_row(row) {}
    
    CursorPosition()
        : m_coloumn(0), m_row(0) {}
    
    inline CursorPosition column_offset(int column) const { return CursorPosition(column, m_row); }

    inline int& coloumn() { return m_coloumn; }
    inline int& row() { return m_row; }
    inline int coloumn() const { return m_coloumn; }
    inline int row() const { return m_row; }

    inline void move_to(int coloumn, int row) 
    { 
        m_coloumn = coloumn;
        m_row = row;
    }
    inline void move_by(int coloumn, int row)
    {
        m_coloumn += coloumn;
        m_row += row;
    }

    bool operator ==(const CursorPosition &other) const
    {
        return m_row == other.m_row && m_coloumn == other.m_coloumn;
    }
    bool operator !=(const CursorPosition &other) const { return !(*this == other); }
    
private:
    int m_coloumn { 0 };
    int m_row { 0 };
    
};
