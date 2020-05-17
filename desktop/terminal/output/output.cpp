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

void Output::out_escape(std::unique_ptr<Escape::Sequence> escape_sequence)
{
    switch (escape_sequence->type())
    {
        case Escape::Sequence::Attribute:
        {
            auto attributes = static_cast<Escape::Attribute&>(*escape_sequence);
            for (const auto &color : attributes.colors())
                line_at(m_cursor).set_attribute(m_cursor.coloumn(), color.first, color.second);
            for (const auto &flag : attributes.flags())
                line_at(m_cursor).set_attribute(m_cursor.coloumn(), flag.first, flag.second);              
            break;
        }
        
        case Escape::Sequence::Cursor:
        {
            auto cursor = static_cast<Escape::Cursor&>(*escape_sequence);
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
        
        case Escape::Sequence::SetCursor:
        {
            auto set_cursor = static_cast<Escape::SetCursor&>(*escape_sequence);
            auto column = std::max(set_cursor.coloumn() - 1, 0);
            auto row = std::max(set_cursor.row() - 1, 0);
            
            m_cursor.move_to(column, row + m_curr_frame_index);
            break;
        }
        
        case Escape::Sequence::Insert:
        {
            auto insert = static_cast<Escape::Insert&>(*escape_sequence);
            m_insert_count = insert.count();
            break;
        }
        
        case Escape::Sequence::Delete:
        {
            auto del = static_cast<Escape::Delete&>(*escape_sequence);
            line_at(m_cursor).erase(m_cursor.coloumn() - del.count() + 1, del.count());
            break;
        }
        
        case Escape::Sequence::Clear:
        {
            auto clear = static_cast<Escape::Clear&>(*escape_sequence);
            
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
                auto &sequence = std::get<std::unique_ptr<Escape::Sequence>>(result.value);
                out_escape(std::move(sequence)); 
                break;
            }
            default: break;
        }
    }
    
    draw_window();
}
