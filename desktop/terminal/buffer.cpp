#include "buffer.hpp"
#include "decoder.hpp"
#include <iostream>

Buffer::Rune Buffer::blank_rune()
{
    return Rune { ' ', {} };
}

Buffer::Buffer(int rows, int columns)
    : m_rows(rows)
    , m_columns(columns)
{
    m_buffer = allocate_new_buffer(m_buffer, rows, columns, m_columns, m_rows);
}

Buffer::Rune *Buffer::allocate_new_buffer(Rune *old_buffer,
    int new_rows, int new_columns,
    int old_rows, int old_columns)
{
    Rune *new_buffer = (Rune*)malloc(new_rows * new_columns * sizeof(Rune));
    if (m_buffer == nullptr)
    {
        for (int i = 0; i < new_rows * new_columns; i++)
            new_buffer[i] = blank_rune();
        return new_buffer;
    }

    for (int row = 0; row < new_rows; row++)
    {
        for (int column = 0; column < new_columns; column++)
        {
            auto new_index = row * new_columns + column;
            auto old_index = row * old_columns + column;
            if (row < old_rows && column < old_columns)
                new_buffer[new_index] = old_buffer[old_index];
            else
                new_buffer[new_index] = blank_rune();
        }
    }

    free(old_buffer);
    return new_buffer;
}

void Buffer::resize(int rows, int columns)
{
    if (rows == m_rows && columns == m_columns)
        return;

    m_buffer = allocate_new_buffer(m_buffer, rows, columns, m_rows, m_columns);
    m_scroll_back = allocate_new_buffer(m_scroll_back, m_scroll_back_rows, columns, m_scroll_back_rows, m_columns);
    m_rows = rows;
    m_columns = columns;
}

void Buffer::clear_row(int row)
{
    assert (row >= 0 && row < m_rows);

    for (int i = 0; i < m_columns; i++)
        m_buffer[row * m_columns + i] = blank_rune();
}

void Buffer::scroll(int top, int bottom, int by)
{
    auto start_index = top * m_columns;
    auto end_index = (bottom + 1) * m_columns;
    auto by_index = by * m_columns;
    if (by > 0)
    {
        // Copy overflow into scrollback buffer
        m_scroll_back = allocate_new_buffer(m_scroll_back, m_scroll_back_rows + by, m_columns, m_scroll_back_rows, m_columns);
        for (int i = 0; i < by_index; i++)
            m_scroll_back[m_scroll_back_rows * m_columns + i] = m_buffer[i];
        m_scroll_back_rows += by;

        for (int i = start_index; i < end_index - start_index - by; i++)
            m_buffer[i] = m_buffer[i + by_index];
        for (int i = bottom - by + 1; i < bottom + 1; i++)
            clear_row(i);
    }
    else if (by < 0)
    {
        for (int i = end_index - start_index; i > start_index - by_index; i--)
            m_buffer[i] = m_buffer[i + by_index];

        if (m_scroll_back_rows >= -by)
        {
            // Copy scrollback, back into buffer
            for (int i = 0; i < -by_index; i++)
                m_buffer[i + start_index] = m_scroll_back[(m_scroll_back_rows + by) * m_columns + i];

            // Shrink scrollback
            m_scroll_back = (Rune*)realloc(m_scroll_back, ((m_scroll_back_rows + by) * m_columns) * sizeof (Rune));
            m_scroll_back_rows += by;
        }
        else
        {
            // Clear top lines
            for (int i = top; i < top - by; i++)
                clear_row(i);
        }
    }
}

Buffer::Rune &Buffer::rune_at(const CursorPosition &pos)
{
    if (!m_buffer)
        m_buffer = allocate_new_buffer(m_buffer, m_rows, m_columns, m_rows, m_columns);

    if (!(pos.row() >= 0 && pos.row() < m_rows &&
            pos.coloumn() >= 0 && pos.coloumn() < m_columns))
    {
        std::cout << "Invalid access {row=" << pos.row() << ", column=" << pos.coloumn() << "}\n";
        std::cout << "Buffer size {rows=" << rows() << ", columns=" << columns() << "}\n";
        return m_invalid_rune;
    }

    return m_buffer[pos.row() * m_columns + pos.coloumn()];
}

const Buffer::Rune &Buffer::rune_at_scroll_offset(const CursorPosition &pos, int offset) const
{
    auto row = pos.row() + offset;
    if (row >= 0)
        return rune_at(CursorPosition(pos.coloumn(), row));

    if (-row >= m_scroll_back_rows)
    {
        std::cout << "Invalid scroll offset " << row << ", scroll_back_rows=" << m_scroll_back_rows << "\n";
        return m_invalid_rune;
    }
    return m_scroll_back[(m_scroll_back_rows + row) * m_columns + pos.coloumn()];
}

const Buffer::Rune &Buffer::rune_at(const CursorPosition &pos) const
{
    return const_cast<Buffer*>(this)->rune_at(pos);
}
