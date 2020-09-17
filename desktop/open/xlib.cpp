#include "xlib.hpp"
#include <iostream>

Visual *XLibOutput::load_double_buffering()
{
    int num_of_screens = 0;
    auto *screens = &DefaultRootWindow(m_display);

    XdbeScreenVisualInfo *screen_info = XdbeGetVisualInfo(
        m_display, screens, &num_of_screens);
    if (!screen_info || num_of_screens < 1 || screen_info->count < 1)
    {
        std::cerr << "No visuals support Xdbe\n";
        return nullptr;
    }

    XVisualInfo info = {};
    info.visualid = screen_info->visinfo[0].visual;
    info.screen = 0;
    info.depth = screen_info->visinfo[0].depth;

    int matches;
    XVisualInfo *match = XGetVisualInfo(m_display,
        VisualIDMask|VisualScreenMask|VisualDepthMask,
        &info, &matches);

    if (matches == 0)
    {
        std::cerr << "No visuals support Xdbe\n";
        return nullptr;
    }

    return match->visual;
}

void XLibOutput::load_font(const std::string &name, int size)
{
    m_font = XftFontOpenName(m_display, m_screen,
        (name + ":size=" + std::to_string(size) + ":antialias=true").c_str());
    if (!m_font)
    {
        std::cerr << "terminal: xft: Couln't load font\n";
        return;
    }

    if (!XftColorAllocName(m_display, m_visual, m_color_map, "#FFFFFF", &m_color))
    {
        std::cerr << "Could not allocate font color\n";
        return;
    }

    m_draw = XftDrawCreate(m_display, m_back_buffer, m_visual, m_color_map);
    if (!m_draw)
    {
        std::cerr << "terminal: xft: Could not create draw\n";
        return;
    }
}

XLibOutput::XLibOutput()
{
    int width = 800, height = 600;
    m_display = XOpenDisplay(nullptr);
    m_screen = DefaultScreen(m_display);
    m_visual = load_double_buffering();

    XSetWindowAttributes window_attr = {};
    window_attr.border_pixel = 0;
    m_window = XCreateWindow(
        m_display, DefaultRootWindow(m_display),
        10, 10, width, height, 0,
        CopyFromParent, CopyFromParent, m_visual,
        CWBackPixel | CWColormap | CWBorderPixel, &window_attr);
    XStoreName(m_display, m_window, "open");

    m_back_buffer = XdbeAllocateBackBufferName(m_display, m_window, XdbeCopied);
    m_color_map = XCreateColormap(m_display, m_window, m_visual, 0);
    m_gc = XCreateGC(m_display, m_back_buffer, 0, 0);
    load_font("Sans Regular", 16);

    XSelectInput(m_display, m_window, ExposureMask);
    XMapWindow(m_display, m_window);
}

int XLibOutput::run()
{
    XEvent event;
    while (XNextEvent(m_display, &event) == 0)
    {
        switch (event.type)
        {
            case Expose:
                on_draw();
                break;
        }
    }

    return 0;
}

void XLibOutput::on_draw()
{
    XSetForeground(m_display, m_gc, 0xFFFFFF);
    XFillRectangle(m_display, m_back_buffer, m_gc, 10, 10, 100, 100);

    XftDrawStringUtf8(m_draw, &m_color, m_font, 120, 50, (FcChar8*)"Hello!", 6);

    XdbeSwapInfo swap_info = { m_window, XdbeCopied };
    if (!XdbeSwapBuffers(m_display, &swap_info, 1))
        std::cerr << "Unable to swap buffers\n";
    XFlush(m_display);
}
