#pragma once
#include "attribute.hpp"
#include "cursor.hpp"
#include <stdint.h>

class Buffer
{
public:
    struct Rune
    {
        uint32_t value;
        Attribute attribute;
    };

    static Rune blank_rune();

    Buffer() = default;
    Buffer(int rows, int columns);

    void resize(int rows, int columns);
    void clear_row(int row);
    void scroll(int top, int botton, int by);
    Rune &rune_at(const CursorPosition&);
    const Rune &rune_at_scroll_offset(const CursorPosition&, int offset) const;
    const Rune &rune_at(const CursorPosition&) const;

    inline int rows() const { return m_rows; }
    inline int columns() const { return m_columns; }
    inline int scroll_back() const { return m_scroll_back_rows; }

private:
    Rune *m_buffer { nullptr };
    Rune *m_scroll_back { nullptr };
    Rune m_invalid_rune { ' ', {} };
    int m_rows { 80 };
    int m_columns { 80 };
    int m_scroll_back_rows { 0 };

    Rune *allocate_new_buffer(Rune *buffer,
        int new_rows, int new_columns,
        int old_rows, int old_columns);
};
