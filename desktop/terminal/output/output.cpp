#include "output.hpp"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <memory.h>
#include <unistd.h>

#define DEFAULT(a, b) (a ? a : b)
// #define DEBUG_OUTPUT

void Output::resize(int rows, int columns)
{
    m_buffer.resize(rows, columns);
    m_scroll_region_top = 0;
    m_scroll_region_bottom = rows - 1;

    move_cursor_to(0, m_cursor.row());
    m_buffer.clear_row(m_cursor.row());

    redraw_all();
    flush_display();
}

void Output::move_cursor_to(int column, int row)
{
    auto min_row = 0;
    auto max_row = rows() - 1;
    if (m_relative_origin_mode)
    {
        row += m_scroll_region_top;
        min_row = m_scroll_region_top;
        max_row = m_scroll_region_bottom;
    }

    if (row >= rows())
    {
        row = rows() - 1;
        scroll(rows() - row);
    }

    if (m_auto_wrap_mode)
    {
        if (column < 0)
            column = 0;
        if (row < 0)
            row = 0;

        auto row_offset = (int)(column / (columns() - 1));
        m_cursor.move_to(
            column % (columns() - 1),
            std::clamp(row + row_offset, min_row, max_row));
    }
    else
    {
        m_cursor.move_to(
            std::clamp(column, 0, columns() - 1),
            std::clamp(row, min_row, max_row));
    }
}

void Output::move_cursor_by(int column, int row)
{
    move_cursor_to(
        m_cursor.coloumn() + column,
        m_cursor.row() + row);
}

void Output::out_rune(uint32_t rune)
{
    if (rune == '\r' || rune == '\033')
        return;
    
    if (rune == '\n')
    {
        move_cursor_to(0, m_cursor.row() + 1);
    }
    else
    {
#ifdef DEBUG_OUTPUT
        std::cout << "Out '" << (char)rune << "' (" << rune << ") "
            "at " << m_cursor.row() << "," << m_cursor.coloumn() << "\n";
#endif
        m_buffer.rune_at(m_cursor) = { rune, m_current_attribute };
        draw_rune(m_cursor);
        move_cursor_by(1, 0);
    }
    
    if (m_cursor.row() < m_scroll_region_top)
        scroll(m_cursor.row() - m_scroll_region_top);

    if (m_cursor.row() > m_scroll_region_bottom)
        scroll(m_cursor.row() - m_scroll_region_bottom);
}

void Output::set_mode(int mode, bool value)
{
    std::cout << "Set mode " << mode << ": " << value << "\n";
    switch (mode)
    {
        case 1:
            m_application_keys_mode = value;
            break;
        case 6:
            m_relative_origin_mode = value;

            // NOTE: We should always move to 0,0 when setting this
            move_cursor_to(0, 0);
            break;
        case 7:
            m_auto_wrap_mode = value;
            break;
    }
}

void Output::wait()
{
    draw_rune(m_cursor, true);
    flush_display();
    usleep(10 * 1000);
    draw_rune(m_cursor, false);
}

void Output::out_escape(Decoder::EscapeSequence &escape)
{
    int arg_len = escape.args.size();

    switch (escape.command)
    {
        case 'm':
        {
            if (arg_len == 0)
            {
                m_current_attribute.apply(0);
                break;
            }

            for (int attr_code : escape.args)
                m_current_attribute.apply(attr_code);
            break;
        }
        
        case 'A':
        case 'M':
            move_cursor_by(0, arg_len ? DEFAULT(-escape.args[0], 1) : -1);
            break;

        case 'B':
            move_cursor_by(0, arg_len ? DEFAULT(escape.args[0], 1) : 1);
            break;

        case 'D':
            if (arg_len == 0)
            {
                move_cursor_by(0, 1);
                break;
            }

            assert (arg_len == 1);
            move_cursor_by(DEFAULT(-escape.args[0], 1), 0);
            break;

        case 'C':
            move_cursor_by(arg_len ? DEFAULT(escape.args[0], 1) : 1, 0);
            break;
        
        case 'f':
        case 'H':
        {
            assert (arg_len <= 2);
            auto row =  arg_len >= 1 ? escape.args[0] : 1;
            auto column =  arg_len >= 2 ? escape.args[1] : 1;

            move_cursor_to(column - 1, row - 1);
            break;
        }

        // Cursor Character Absolute [column] (default = [row,1]) (CHA)
        case 'G':
        {
            assert (arg_len <= 1);
            auto column = arg_len >= 1 ? escape.args[0] : 1;

            move_cursor_to(column - 1, m_cursor.row());
            break;
        }
        
        // Line Position Absolute [row] (default = [1,column]) (VPA)
        case 'd':
        {
            assert (arg_len <= 1);
            auto row = arg_len >= 1 ? escape.args[0] : 1;

            move_cursor_to(m_cursor.coloumn(), row - 1);
            break;
        }

        // Move to next line
        case 'E':
        {
            assert(arg_len == 0);
            move_cursor_to(0, m_cursor.row() + 1);
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
            for (int i = m_cursor.coloumn() + del_count; i < columns(); i++)
            {
                auto to = m_cursor.column_offset(i - del_count);
                auto from = m_cursor.column_offset(i);
                m_buffer.rune_at(to) = m_buffer.rune_at(from);
                draw_rune(CursorPosition(i - del_count, m_cursor.row()));
            }
            break;
        }

        case 'K':
        {
            assert (arg_len <= 1);
            auto mode = arg_len ? escape.args[0] : 0;

            switch (mode)
            {
                // Clear line from cursor right
                case 0:
                    for (int i = m_cursor.coloumn(); i < columns(); i++)
                    {
                        m_buffer.rune_at(m_cursor.column_offset(i)).value = ' ';
                        draw_rune(CursorPosition(i, m_cursor.row()));
                    }
                    break;

                // Clear line from cursor left
                case 1:
                    for (int i = 0; i <= m_cursor.coloumn(); i++)
                    {
                        m_buffer.rune_at(m_cursor.column_offset(i)).value = ' ';
                        draw_rune(CursorPosition(i, m_cursor.row()));
                    }
                    break;

                // Clear entire line
                case 2:
                    m_buffer.clear_row(m_cursor.row());
                    for (int i = 0; i < columns(); i++)
                        draw_rune(CursorPosition(i, m_cursor.row()));
                    break;

                default:
                    assert (false);
            }
            break;
        }

        case 'J':
        {
            assert (arg_len <= 1);

            auto mode = arg_len ? escape.args[0] : 0;
            switch(mode)
            {
                // Clear screen from cursor down
                case 0:
                    for (int i = m_cursor.row(); i < rows(); i++)
                        m_buffer.clear_row(i);
                    redraw_all();
                    break;

                // Clear screen from cursor up
                case 1:
                    for (int i = 0; i <= m_cursor.row(); i++)
                        m_buffer.clear_row(i);
                    redraw_all();
                    break;

                // Clear entire screen
                case 2:
                    for (int i = 0; i < rows(); i++)
                        m_buffer.clear_row(i);
                    move_cursor_to(0, 0);
                    redraw_all();
                    break;
            }
            break;
        }

        case 'c':
        {
            assert (arg_len <= 1);
            input("\033[?6c");
            break;
        }

        case 'r':
        {
            if (arg_len == 0)
            {
                m_scroll_region_top = 0;
                m_scroll_region_bottom = rows() - 1;
                break;
            }

            assert (arg_len == 2);
            m_scroll_region_top = escape.args[0] - 1;
            m_scroll_region_bottom = escape.args[1] - 1;
            break;
        }

        case 'L':
        {
            assert (arg_len <= 1);
            scroll(arg_len ? -escape.args[0] : -1);
            break;
        }

        case 'l':
            assert (arg_len == 1);
            set_mode(escape.args[0], false);
            break;

        case 'h':
            assert (arg_len == 1);
            set_mode(escape.args[0], true);
            break;

        // Screen alignment display
        case '#':
            assert (arg_len == 1);
            switch (escape.args[0])
            {
                case 8:
                    for (int row = 0; row < rows(); row++)
                    {
                        for (int column = 0; column < columns(); column++)
                            m_buffer.rune_at(CursorPosition(column, row)) = { 'E', m_current_attribute };
                    }
                    break;
            }
            break;

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

void Output::out_tab()
{
    int length = 8 - (m_cursor.coloumn() % 8);
    for (int i = 0; i < length; i++)
        out_rune(' ');
}

void Output::out(std::string_view buff)
{
    for (int i = 0; i < (int)buff.length(); i++)
    {
        char c = buff[i];
        if (m_insert_count > 0)
        {
            for (int i = columns() - 1; i > m_cursor.coloumn(); i--)
            {
                auto from = m_cursor.column_offset(i - 1);
                auto to = m_cursor.column_offset(i);
                m_buffer.rune_at(from) = m_buffer.rune_at(to);
                draw_rune(CursorPosition(i, m_cursor.row()));
            }
            m_buffer.rune_at(m_cursor).value = c;
            m_insert_count -= 1;
        }
        
        auto result = m_decoder.parse(c);
        
        switch (result.type)
        {
            case Decoder::Result::Rune: out_rune(result.value); break;
            case Decoder::Result::Bell: std::cout << "BELL!\n"; break;
            case Decoder::Result::Escape: out_escape(result.escape); break;
            case Decoder::Result::Tab: out_tab(); break;
            default: break;
        }
    }

    draw_rune(m_last_cursor);
    draw_rune(m_cursor, true);
    m_last_cursor = m_cursor;
    flush_display();
}

void Output::scroll(int by)
{
    draw_rune(m_last_cursor);
    m_buffer.scroll(m_scroll_region_top, m_scroll_region_bottom, by);
    draw_scroll(m_scroll_region_top, m_scroll_region_bottom, by);
}
