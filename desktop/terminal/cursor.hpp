#pragma once
#include <algorithm>

class CursorPosition
{
public:
    CursorPosition(int coloumn, int row)
        : m_coloumn(coloumn), m_row(row) {}
    
    CursorPosition()
        : m_coloumn(0), m_row(0) {}
    
    inline int& coloumn() { return m_coloumn; }
    inline int& row() { return m_row; }
    inline int coloumn() const { return m_coloumn; }
    inline int row() const { return m_row; }

    inline void move_to(int coloumn, int row) 
    { 
        m_coloumn = std::max(coloumn, 0); 
        m_row = std::max(row, 0);
    }
    inline void move_to_begging_of_line() { m_coloumn = 0; }

    inline void move_by(int coloumn, int row) 
    {
        m_coloumn = std::max(m_coloumn + coloumn, 0);
        m_row = std::max(m_row + row, 0);
    }
    
private:
    int m_coloumn { 0 };
    int m_row { 0 };
    
};
