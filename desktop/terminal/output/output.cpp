#include "output.hpp"
#include "../escapes.hpp"
#include <iostream>

Line &Output::line_at(const CursorPosition &position)
{
    if (position.row() < 0)
    {
        if (m_lines.size() == 0)
            m_lines.emplace_back();
        return m_lines[0];
    }
    
    while (position.row() >= m_lines.size())
        m_lines.emplace_back();
    
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
    
    return line_at(CursorPosition(0, row)).data().length();
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

void Output::out(std::string_view buff)
{
    for (int i = 0; i < buff.length(); i++)
    {
        char c = buff[i];
        auto escape_sequence = Escape::Sequence::parse(
            std::string_view(buff.data() + i, buff.length() - i));
        
        if (!escape_sequence)
        {
            if (c == '\n')
            {
                m_cursor.move_by(0, 1);
                m_cursor.move_to_begging_of_line();
            }
            else
            {
                line_at(m_cursor).set(m_cursor.coloumn(), c);
                m_cursor.move_by(1, 0);
                
                if (m_cursor.row() >= m_curr_frame_index + m_rows)
                    scroll(m_cursor.row() - (m_curr_frame_index + m_rows - 1));
            }
            
            continue;
        }
        
        std::cout << "Escape: " << *escape_sequence << "\n";
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
                    
                    case Escape::Cursor::TopLeft:
                        m_cursor.move_to(0, 0);
                        break;
                    
                    default:
                        break;
                }
                line_at(m_cursor).mark_dirty();
                break;
            }
            
            case Escape::Sequence::Insert:
            {
                auto insert = static_cast<Escape::Insert&>(*escape_sequence);
                if (i + escape_sequence->char_count() + insert.count() < buff.length())
                {
                    auto str = std::string_view(
                        buff.data() + i + escape_sequence->char_count() + 1, insert.count());

                    for (char c : str)
                        line_at(m_cursor).insert(m_cursor.coloumn(), c);
                    i += insert.count();
                    m_cursor.move_by(insert.count(), 0);
                }
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
                        while (index < line.data().length())
                        {
                            line.set(index, ' ');
                            index += 1;
                        }
                        break;
                    }
                    
                    case Escape::Clear::Screen:
                        m_lines.clear();
                        m_curr_frame_index = 0;
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
        
        i += escape_sequence->char_count();
    }
    
    draw_window();
}
