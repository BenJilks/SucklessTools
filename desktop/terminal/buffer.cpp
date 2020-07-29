#include "buffer.hpp"
#include "decoder.hpp"
#include <iostream>

Buffer::Rune Buffer::blank_rune()
{
    return Rune { ' ', TerminalColor::default_color() };
}

Buffer::Buffer(int rows, int columns)
    : m_rows(rows)
    , m_columns(columns)
{
    allocate_new_buffer(rows, columns);
}

void Buffer::allocate_new_buffer(int rows, int columns)
{
    int old_buffer_size = m_rows * m_columns * sizeof(Rune);
    int new_buffer_size = rows * columns * sizeof(Rune);
    if (!m_buffer)
    {
        old_buffer_size = 0;
        m_buffer = (Rune*)malloc(new_buffer_size);
    }
    else
        m_buffer = (Rune*)realloc(m_buffer, new_buffer_size);

    for (int i = old_buffer_size; i < new_buffer_size / (int)sizeof(Rune); i++)
        m_buffer[i] = blank_rune();
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

void Buffer::set_attributes(const CursorPosition &pos, int attr_code)
{
    auto on_color = [&](auto type, auto new_color)
    {
        auto start_index = pos.row() * m_columns + pos.coloumn();
        auto original_color = rune_at(pos).color.from_type(type);

        for (int i = start_index; i < m_rows * m_columns; i++)
        {
            auto &color = m_buffer[i].color.from_type(type);
            if (color != original_color)
                break;

            color = new_color;
        }
    };

    Decoder::parse_attribute_code(attr_code, on_color);
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
