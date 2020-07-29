#include "output.hpp"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <memory.h>

void Output::resize_buffer(int rows, int columns)
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
        m_buffer[i] = Rune { ' ', TerminalColor::default_color() };
}

void Output::resize(int rows, int columns)
{
    std::cout << "set size\n";
    if (m_rows == rows)
        return;

    std::cout << "Resize: (" << rows << ", " << columns << ")\n";
    resize_buffer(rows, columns);
    m_rows = rows;
    m_columns = columns;
    m_scroll_region_top = 0;
    m_scroll_region_bottom = rows - 1;

    m_cursor.move_to(0, m_cursor.row());
    clear_row(m_cursor.row());

    redraw_all();
    flush_display();
}

Rune &Output::rune_at(const CursorPosition &position)
{
    if (!m_buffer)
        resize_buffer(m_rows, m_columns);

    assert (position.row() >= 0 && position.row() < m_rows &&
            position.coloumn() >= 0 && position.coloumn() < m_columns);

    return m_buffer[position.row() * m_columns + position.coloumn()];
}

/*
int Output::line_selection_start(int row)
{
    auto start = m_selection_start;
    auto end = m_selection_end;
    if (end.row() < start.row())
        std::swap(start, end);
    
    if (row == start.row() && row == end.row())
        return std::min(start.coloumn(), end.coloumn());
    
    if (row == start.row())
        return start.coloumn();
    
    return 0;
}

int Output::line_selection_end(int row)
{
    auto start = m_selection_start;
    auto end = m_selection_end;
    if (end.row() < start.row())
        std::swap(start, end);
    
    if (row == start.row() && row == end.row())
        return std::max(start.coloumn(), end.coloumn());
    
    if (row == end.row())
        return end.coloumn();
    
    return line_at(CursorPosition(0, row)).length();
}

bool Output::line_in_selection(int row)
{
    if (!m_has_selection)
        return false;
    
    auto start = m_selection_start.row();
    auto end = m_selection_end.row();
    if (end < start)
        std::swap(start, end);
    
    return row >= start && row <= end;
}
*/

void Output::clear_row(int row)
{
    for (int i = 0; i < m_columns; i++)
        m_buffer[row * m_columns + i] = Rune { ' ', TerminalColor::default_color() };
}

void Output::out_rune(uint32_t rune)
{
    if (rune == '\r' || rune == '\033')
        return;
    
    if (rune == '\n')
    {
        m_cursor.move_to(0, m_cursor.row() + 1);
    }
    else
    {
        rune_at(m_cursor).value = rune;
        draw_rune(m_cursor);
        m_cursor.move_by(1, 0);
    }
    
    if (m_cursor.row() < m_scroll_region_top)
        scroll(-(m_scroll_region_top - m_cursor.row()));

    if (m_cursor.row() > m_scroll_region_bottom)
        scroll(m_cursor.row() - m_scroll_region_bottom);
}

/*
void Output::set_attribute(const CursorPosition&, TerminalColor::Type type, TerminalColor::Named color)
{
    auto &line = line_at(m_cursor);
    line.set_attribute(m_cursor.coloumn(), type, color);
}

void Output::set_attribute(const CursorPosition&, TerminalColor::Flags flag, bool value)
{
    auto &line = line_at(m_cursor);
    line.set_attribute(m_cursor.coloumn(), flag, value);
}
*/

void Output::out_escape(Decoder::EscapeSequence &escape)
{
    int arg_len = escape.args.size();

    switch (escape.command)
    {
        case 'm':
        {
            // TODO: Set attribute
            break;
        }
        
        case 'A': m_cursor.move_by(0, arg_len ? -escape.args[0] : -1); break;
        case 'B': m_cursor.move_by(0, arg_len ? escape.args[0] : 1); break;
        case 'C': m_cursor.move_by(arg_len ? escape.args[0] : 1, 0); break;
        case 'D': m_cursor.move_by(arg_len ? -escape.args[0] : -1, 0); break;
        
        case 'H':
        {
            assert (arg_len <= 2);
            auto row =  arg_len >= 1 ? escape.args[0] : 1;
            auto column =  arg_len >= 2 ? escape.args[1] : 1;

            m_cursor.move_to(column - 1, row - 1);
            break;
        }
        
        case '@':
        {
            assert (arg_len <= 1);
            m_insert_count = arg_len ? escape.args[0] : 1;
            break;
        }
        
        case 'P':
        {
            assert (arg_len <= 1);

            auto del_count = arg_len ? escape.args[0] : 1;
            for (int i = m_cursor.coloumn() + del_count; i < m_columns; i++)
            {
                m_buffer[m_cursor.row() * m_columns + i - del_count] = m_buffer[m_cursor.row() * m_columns + i];
                draw_rune(CursorPosition(i - del_count, m_cursor.row()));
            }
            break;
        }

        case 'K':
        {
            assert (arg_len == 0);

            for (int i = m_cursor.coloumn(); i < m_columns; i++)
            {
                m_buffer[m_cursor.row() * m_columns + i].value = ' ';
                draw_rune(CursorPosition(i, m_cursor.row()));
            }
            break;
        }

        case 'J':
        {
            assert (arg_len && escape.args[0] == 2);

            for (int i = 0; i < m_rows; i++)
                clear_row(i);
            m_cursor.move_to(0, 0);
            redraw_all();
            break;
        }

        case 'r':
        {
            assert (arg_len == 2);
            m_scroll_region_top = escape.args[0] - 1;
            m_scroll_region_bottom = escape.args[1] - 1;
            break;
        }

        case 'L':
        {
            assert (arg_len == 0);
            scroll(-1);
            break;
        }

        default:
            if (escape.command == 'l' || escape.command == 'h')
                break;
#if 1
            std::cout << "Unkown Escape: " << escape.command << "(";
            for (int arg : escape.args)
                std::cout << arg << ", ";
            std::cout << ")\n";
#endif
            break;
    }
}

void Output::out(std::string_view buff)
{
    for (int i = 0; i < (int)buff.length(); i++)
    {
        char c = buff[i];
        if (m_insert_count > 0)
        {
            for (int i = m_columns - 1; i > m_cursor.coloumn(); i--)
            {
                m_buffer[m_cursor.row() * m_columns + i] = m_buffer[m_cursor.row() * m_columns + i - 1];
                draw_rune(CursorPosition(i, m_cursor.row()));
            }
            rune_at(m_cursor).value = c;
            m_insert_count -= 1;
        }
        
        auto result = m_decoder.parse(c);
        
        switch (result.type)
        {
            case Decoder::Result::Rune: out_rune(result.value); break;
            case Decoder::Result::Bell: std::cout << "BELL!\n"; break;
            case Decoder::Result::Escape: out_escape(result.escape); break;
            default: break;
        }
    }

    draw_rune(m_last_cursor);
    draw_cursor();
    m_last_cursor = m_cursor;
    flush_display();
}

void Output::scroll(int by)
{
    draw_rune(m_last_cursor);

    auto start_index = m_scroll_region_top * m_columns;
    auto end_index = (m_scroll_region_bottom + 1) * m_columns;
    auto by_index = by * m_columns;
    if (by > 0)
    {
        for (int i = start_index; i < end_index - start_index - by; i++)
            m_buffer[i] = m_buffer[i + by_index];
        for (int i = m_scroll_region_bottom - by + 1; i < m_scroll_region_bottom + 1; i++)
            clear_row(i);
    }
    else if (by < 0)
    {
        for (int i = end_index - start_index; i > start_index - by; i--)
            m_buffer[i] = m_buffer[i + by_index];
        for (int i = m_scroll_region_top; i < m_scroll_region_top - by; i++)
            clear_row(i);
    }

    //draw_scroll(m_scroll_region_top, m_scroll_region_bottom, by);

    redraw_all();
    m_cursor.move_by(0, -by);
    m_last_cursor = m_cursor;
}
