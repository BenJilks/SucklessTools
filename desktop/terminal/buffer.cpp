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
    allocate_new_buffer(rows, columns);
}

void Buffer::allocate_new_buffer(int new_rows, int new_columns)
{
    Rune *new_buffer = (Rune*)malloc(new_rows * new_columns * sizeof(Rune));
    if (m_buffer == nullptr)
    {
        for (int i = 0; i < new_rows * new_columns; i++)
            new_buffer[i] = blank_rune();
        m_buffer = new_buffer;
        return;
    }

    const auto old_rows = m_rows, old_columns = m_columns;
    for (int row = 0; row < new_rows; row++)
    {
        for (int column = 0; column < new_columns; column++)
        {
            auto new_index = row * new_columns + column;
            auto old_index = row * old_columns + column;
            if (row < old_rows && column < old_columns)
                new_buffer[new_index] = m_buffer[old_index];
            else
                new_buffer[new_index] = blank_rune();
        }
    }

    free(m_buffer);
    m_buffer = new_buffer;
}

void Buffer::resize(int rows, int columns)
{
    if (rows == m_rows && columns == m_columns)
        return;

    allocate_new_buffer(rows, columns);
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
        for (int i = start_index; i < end_index - start_index - by; i++)
            m_buffer[i] = m_buffer[i + by_index];
        for (int i = bottom - by + 1; i < bottom + 1; i++)
            clear_row(i);
    }
    else if (by < 0)
    {
        for (int i = end_index - start_index; i > start_index - by; i--)
            m_buffer[i] = m_buffer[i + by_index];
        for (int i = top; i < top - by; i++)
            clear_row(i);
    }
}

Buffer::Rune &Buffer::rune_at(const CursorPosition &pos)
{
    if (!m_buffer)
        allocate_new_buffer(m_rows, m_columns);

    if (!(pos.row() >= 0 && pos.row() < m_rows &&
            pos.coloumn() >= 0 && pos.coloumn() < m_columns))
    {
        std::cout << "Invalid access {row=" << pos.row() << ", column=" << pos.coloumn() << "}\n";
        assert(false);
    }

    return m_buffer[pos.row() * m_columns + pos.coloumn()];
}

const Buffer::Rune &Buffer::rune_at(const CursorPosition &pos) const
{
    return const_cast<Buffer*>(this)->rune_at(pos);
}
