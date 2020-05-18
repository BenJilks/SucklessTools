#include "output.hpp"
#include <iostream>
#include <algorithm>

Line &Output::line_at(const CursorPosition &position)
{
    if (position.row() <= 0)
    {
        if (m_lines.size() == 0)
            m_lines.push_back(Line());
        return m_lines[0];
    }
    
    while (position.row() >= m_lines.size())
    {
        Line line;

        if (m_lines.size() > 0)
        {
            auto &last_line = m_lines.back();
            if (last_line.length() > 0)
            {
                auto attr = last_line[last_line.length() - 1].color;
                line.set_attribute(0, attr);
            }
        }
        m_lines.push_back(line);
    }
    
    return m_lines[position.row()];
}

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

void Output::out_rune(uint32_t rune)
{
    if (rune == '\r' || rune == '\033')
        return;
    
    if (rune == '\n')
    {
        line_at(m_cursor).mark_dirty();
        m_cursor.move_by(0, 1);
        m_cursor.move_to_begging_of_line();
        line_at(m_cursor).mark_dirty();
    }
    else
    {
        line_at(m_cursor).set(m_cursor.coloumn(), rune);
        m_cursor.move_by(1, 0);
    }
    
    if (m_cursor.row() > m_curr_frame_index + m_rows - 1)
        scroll(m_cursor.row() - (m_curr_frame_index + m_rows - 1));
}

void Output::set_attribute(const CursorPosition& pos, TerminalColor::Type type, TerminalColor::Named color)
{
    auto &line = line_at(m_cursor);
    line.set_attribute(m_cursor.coloumn(), type, color);
}

void Output::set_attribute(const CursorPosition&, TerminalColor::Flags flag, bool value)
{
    auto &line = line_at(m_cursor);
    line.set_attribute(m_cursor.coloumn(), flag, value);
}

void Output::out_escape(Escape::Sequence &escape_sequence)
{
    switch (escape_sequence.type())
    {
        case Escape::Sequence::Type::Attribute:
        {
            auto attributes = escape_sequence.attribute();
            attributes.for_each_color([this](auto type, auto color)
            {
                set_attribute(m_cursor, type, color);
            });
            attributes.for_each_flag([this](auto flag, auto value)
            {
                set_attribute(m_cursor, flag, value);
            });
            break;
        }
        
        case Escape::Sequence::Type::Cursor:
        {
            auto cursor = escape_sequence.cursor();
            switch (cursor.direction())
            {
                case Escape::Cursor::Up:
                    m_cursor.move_by(0, -cursor.amount());
                    break;

                case Escape::Cursor::Down:
                    m_cursor.move_by(0, cursor.amount());
                    break;

                case Escape::Cursor::Right:
                    m_cursor.move_by(cursor.amount(), 0);
                    break;

                case Escape::Cursor::Left:
                    m_cursor.move_by(-cursor.amount(), 0);
                    break;
                
                default:
                    break;
            }
            line_at(m_cursor).mark_dirty();
            break;
        }
        
        case Escape::Sequence::Type::SetCursor:
        {
            auto set_cursor = escape_sequence.set_cursor();
            auto column = std::max(set_cursor.coloumn() - 1, 0);
            auto row = std::max(set_cursor.row() - 1, 0);
            
            m_cursor.move_to(column, row + m_curr_frame_index);
            break;
        }
        
        case Escape::Sequence::Type::Insert:
        {
            auto insert = escape_sequence.insert();
            m_insert_count = insert.count();
            break;
        }
        
        case Escape::Sequence::Type::Delete:
        {
            auto del = escape_sequence.delete_();
            line_at(m_cursor).erase(m_cursor.coloumn() - del.count() + 1, del.count());
            break;
        }
        
        case Escape::Sequence::Type::Clear:
        {
            auto clear = escape_sequence.clear();
            
            switch (clear.mode())
            {
                case Escape::Clear::CursorToLineEnd: 
                {
                    auto index = m_cursor.coloumn();
                    auto &line = m_lines[m_cursor.row()];
                    while (index < line.length())
                    {
                        line.set(index, ' ');
                        index += 1;
                    }
                    break;
                }
                
                case Escape::Clear::Screen:
                    m_lines.clear();
                    m_curr_frame_index = 0;
                    m_cursor.move_to(0, 0);
                    redraw_all();
                    break;
                
                default:
                    break;
            }
            
            break;
        }
        
        default:
            break;
    }
}

void Output::out(std::string_view buff)
{
    for (int i = 0; i < buff.length(); i++)
    {
        char c = buff[i];
        if (m_insert_count > 0)
        {
            line_at(m_cursor).insert(m_cursor.coloumn(), c);
            m_insert_count -= 1;
        }
        
        auto result = m_decoder.parse(c);
        
        switch (result.type)
        {
            case Decoder::Result::Rune: out_rune(std::get<uint32_t>(result.value)); break;
            case Decoder::Result::Escape:
            {
                auto &sequence = std::get<Escape::Sequence>(result.value);
                out_escape(sequence);
                break;
            }
            default: break;
        }
    }
    
    draw_window();
}
