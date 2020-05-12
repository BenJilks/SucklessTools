#pragma once
#include "output.hpp"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <vector>
#include <iostream>

struct XftPallet
{
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
    virtual void draw_window() override;
    virtual void scroll(int by) override;

    std::string decode_key_press(XKeyEvent *key_event);
    CursorPosition cursor_position_from_pixels(int x, int y);
    void load_font(const std::string &&name, int size);
    XftColor &text_color_from_terminal(TerminalColor color);
    void swap_buffers();
    
    int m_width { 0 };
    int m_height { 0 };
    CursorPosition m_mouse_pos;
    
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
    int m_screen;
    int m_color_map;
    
};
