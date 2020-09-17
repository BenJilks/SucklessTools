#pragma once

#include "output.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <X11/Xft/Xft.h>

class XLibOutput final : public Output
{
public:
    XLibOutput();

    virtual int run() override;

private:
    Display *m_display;
    Window m_window;
    GC m_gc;
    int m_screen;
    XdbeBackBuffer m_back_buffer;
    Visual *m_visual;
    Colormap m_color_map;

    XftFont *m_font;
    XftDraw *m_draw;
    XftColor m_color;

    void on_draw();
    Visual *load_double_buffering();
    void load_font(const std::string &name, int size);

};
