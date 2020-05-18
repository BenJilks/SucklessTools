#pragma once
#include "../line.hpp"
#include "../cursor.hpp"
#include "../decoder.hpp"
#include <string>
#include <functional>
#include <termios.h>
#include <sys/ioctl.h>

class Output
{
public:
    Output() {}

    virtual int input_file() const = 0;
    virtual std::string update() = 0;
    void out(std::string_view str);

    // Hooks
    std::function<void(struct winsize)> on_resize;
    
protected:
    virtual void redraw_all() = 0;
    virtual void draw_window() = 0;
    virtual void scroll(int by) = 0;
    void out_rune(uint32_t);
    void out_escape(Escape::Sequence&);
    void set_attribute(const CursorPosition&, TerminalColor::Type, TerminalColor::Named);
    void set_attribute(const CursorPosition&, TerminalColor::Flags, bool);

    Line &line_at(const CursorPosition &position);
    bool line_in_selection(int row);
    int line_selection_start(int row);
    int line_selection_end(int row);
    
    std::vector<Line> m_lines;
    CursorPosition m_cursor;
    int m_rows { 1024 };
    int m_curr_frame_index { 0 };
    
    Decoder m_decoder;
    int m_insert_count { 0 };

    bool m_in_selection { false };
    bool m_has_selection { false };
    CursorPosition m_selection_start;
    CursorPosition m_selection_end;
    
};
