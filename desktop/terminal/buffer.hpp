#pragma once
#include "color.hpp"
#include "cursor.hpp"
#include <stdint.h>

class Buffer
{
public:
    struct Rune
    {
        uint32_t value;
        TerminalColor color;
    };

    static Rune blank_rune();

    Buffer() = default;
    Buffer(int rows, int columns);

    void resize(int rows, int columns);
    void clear_row(int row);
    void scroll(int top, int botton, int by);
    Rune &rune_at(const CursorPosition&);
    const Rune &rune_at(const CursorPosition&) const;

    inline int rows() const { return m_rows; }
    inline int columns() const { return m_columns; }

private:
    Rune *m_buffer { nullptr };
    int m_rows { 80 };
    int m_columns { 80 };

    void allocate_new_buffer(int rows, int columns);

};
