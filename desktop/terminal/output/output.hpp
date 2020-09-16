#pragma once
#include "../buffer.hpp"
#include "../cursor.hpp"
#include "../decoder.hpp"
#include "../attribute.hpp"
#include <string>
#include <functional>
#include <memory>
#include <termios.h>
#include <sys/ioctl.h>

class Output
{
public:
    Output();

    virtual void init() = 0;
    virtual int input_file() const = 0;
    virtual std::string update() = 0;
    void out(std::string_view str);

    inline bool should_close() const { return m_should_close; }

    // Hooks
    std::function<void(struct winsize)> on_resize;
    
protected:
    enum class RuneMode
    {
        Normal,
        Hilighted,
        Cursor
    };

    virtual void redraw_all() = 0;
    virtual void draw_rune(const CursorPosition&, RuneMode mode = RuneMode::Normal) = 0;
    virtual void draw_scroll(int begin, int end, int by) = 0;
    virtual void flush_display() = 0;
    virtual void input(const std::string&) = 0;
    virtual void out_os_command(Decoder::OSCommand&) {}
    virtual void on_out_rune(uint32_t) {}

    void resize(int rows, int columns);
    void scroll(int by);
    void flush_scroll();
    void set_mode(int mode, bool value);
    void set_should_close(bool b) { m_should_close = b; }

    void move_cursor_to(int column, int row);
    void move_cursor_by(int column, int row);

    inline CursorPosition &cursor() { return m_buffer->cursor(); }
    inline const CursorPosition &cursor() const { return m_buffer->cursor(); }
    inline const Buffer &buffer() const { return *m_buffer; }
    inline int rows() const { return m_buffer->rows(); }
    inline int columns() const { return m_buffer->columns(); }

private:
    std::unique_ptr<Buffer> m_buffer;
    int m_scroll_buffer { 0 };
    bool m_should_close { false };
    CursorPosition m_last_cursor;

    Decoder m_decoder;
    int m_insert_count { 0 };

    void out_rune(uint32_t);
    void out_escape(Decoder::EscapeSequence&);
    void out_tab();
    void wait();

};
