#pragma once
#include "../line.hpp"
#include "../cursor.hpp"
#include <string>

class Output
{
public:
    Output() {}

    virtual int input_file() const = 0;
    virtual std::string update() = 0;
    void out(std::string_view str);

protected:
    virtual void redraw_all() = 0;
    virtual void draw_window() = 0;
    virtual void scroll(int by) = 0;
    Line &line_at(const CursorPosition &position);
    
    std::vector<Line> m_lines;
    CursorPosition m_cursor;
    CursorPosition m_mouse_pos;
    int m_rows { 1024 };
    int m_curr_frame_index { 0 };
    
};
