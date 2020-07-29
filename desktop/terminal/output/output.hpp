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
    virtual void draw_rune(const CursorPosition&) = 0;
    virtual void draw_cursor() = 0;
    virtual void draw_scroll(int begin, int end, int by) = 0;
    virtual void flush_display() = 0;

    void resize(int rows, int columns);
    void scroll(int by);
    void clear_row(int row);

    /*
    void set_attribute(const CursorPosition&, TerminalColor::Type, TerminalColor::Named);
    void set_attribute(const CursorPosition&, TerminalColor::Flags, bool);
    */

    Rune &rune_at(const CursorPosition &position);
//    bool line_in_selection(int row);
//    int line_selection_start(int row);
//    int line_selection_end(int row);

    inline const CursorPosition &cursor() const { return m_cursor; }
    inline int rows() const { return m_rows; }
    inline int columns() const { return m_columns; }

private:
    Rune *m_buffer { nullptr };
    CursorPosition m_cursor;
    CursorPosition m_last_cursor;
    int m_rows { 80 };
    int m_columns { 80 };
    int m_scroll_region_top { 0 };
    int m_scroll_region_bottom { 79 };

    Decoder m_decoder;
    int m_insert_count { 0 };

    bool m_in_selection { false };
    bool m_has_selection { false };
    CursorPosition m_selection_start;
    CursorPosition m_selection_end;

    void resize_buffer(int rows, int columns);
    void out_rune(uint32_t);
    void out_escape(Decoder::EscapeSequence&);

};
