#pragma once
#include "output.hpp"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <vector>
#include <iostream>

struct XftPallet
{
    XftColor default_background;
    XftColor default_foreground;
    
    XftColor black;
    XftColor red;
    XftColor green;
    XftColor yellow;
    XftColor blue;
    XftColor magenta;
    XftColor cyan;
    XftColor white;
};

class XLibOutput final : public Output
{
public:
    XLibOutput();
    
    virtual int input_file() const override;
    virtual std::string update() override;
    
    ~XLibOutput();

private:
    virtual void redraw_all() override;
    virtual void draw_rune(const CursorPosition&, bool selected = false) override;
    virtual void draw_scroll(int begin, int end, int by) override;
    virtual void flush_display() override;
    virtual void input(const std::string&) override;
    void draw_row(int row, bool refresh = false);
    void draw_update_selection(const CursorPosition &new_end_pos);

    template<typename CallbackFunc>
    void for_rune_in_selection(CallbackFunc callback);

    std::string decode_key_press(XKeyEvent *key_event);
    CursorPosition cursor_position_from_pixels(int x, int y);
    void load_font(const std::string &&name, int size);
    XftColor &text_color_from_terminal(TerminalColor color);
    
    int m_width { 0 };
    int m_height { 0 };
    int m_depth { 0 };
    int m_font_width { 0 };
    int m_font_height { 0 };
    int m_scroll_offset { 0 };
    bool m_in_selection { false };
    CursorPosition m_selection_start;
    CursorPosition m_selection_end;
    CursorPosition m_mouse_pos;
    std::string m_input_buffer;
    
    // Xft
    XftFont *m_font;
    XftDraw *m_draw;
    XftPallet m_text_pallet;
    XIC m_input_context;
    
    // Xlib
    Display *m_display;
    Visual *m_visual;
    Window m_window;
    Pixmap m_pixel_buffer;
    GC m_gc;
    int m_screen;
    int m_color_map;
    
};
