#pragma once
#include "../buffer.hpp"
#include "../cursor.hpp"
#include "../decoder.hpp"
#include "../attribute.hpp"
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
    virtual void draw_rune(const CursorPosition&, bool selected = false) = 0;
    virtual void draw_scroll(int begin, int end, int by) = 0;
    virtual void flush_display() = 0;
    virtual void input(const std::string&) = 0;

    void resize(int rows, int columns);
    void scroll(int by);
    void set_mode(int mode, bool value);

    void move_cursor_to(int column, int row);
    void move_cursor_by(int column, int row);

    inline const CursorPosition &cursor() const { return m_cursor; }
    inline const Buffer &buffer() const { return m_buffer; }
    inline const Attribute &current_attribute() const { return m_current_attribute; }
    inline int rows() const { return m_buffer.rows(); }
    inline int columns() const { return m_buffer.columns(); }
    inline bool application_keys_mode() const { return m_application_keys_mode; }

private:
    Buffer m_buffer;
    CursorPosition m_cursor;
    CursorPosition m_last_cursor;
    Attribute m_current_attribute;
    int m_scroll_region_top { 0 };
    int m_scroll_region_bottom { 79 };

    bool m_auto_wrap_mode { false };
    bool m_relative_origin_mode { false };
    bool m_application_keys_mode { false };

    Decoder m_decoder;
    int m_insert_count { 0 };

    void out_rune(uint32_t);
    void out_escape(Decoder::EscapeSequence&);
    void out_tab();
    void wait();

};
