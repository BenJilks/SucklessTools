#include "xlib.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>

#include "../config.hpp"

XLibOutput::XLibOutput()
{
    m_display = XOpenDisplay(nullptr);
    if (!m_display)
    {
        std::cerr << "terminal: xlib: Could not open X11 display\n";
        return;
    }

    m_width = 100;
    m_height = 100;
    m_screen = DefaultScreen(m_display);
    m_depth = 32;
    
    XVisualInfo visual_info;
    if (!XMatchVisualInfo(m_display, m_screen, m_depth, TrueColor, &visual_info))
    {
        std::cerr << "terminal: xlib: 32bit depth not supported\n";
        return;
    }
    m_visual = visual_info.visual;
    m_color_map = XCreateColormap(m_display, 
        DefaultRootWindow(m_display), m_visual, AllocNone);
    
    XSetWindowAttributes window_attr;
    window_attr.colormap = m_color_map;
    window_attr.background_pixel = 0xFFFFFF;
    window_attr.border_pixel = 0;

    auto window_mask = CWBackPixel | CWColormap | CWBorderPixel;
    m_window = XCreateWindow(
        m_display, RootWindow(m_display, m_screen), 
        10, 10, m_width, m_height, 0, 
        m_depth, InputOutput, m_visual,
        window_mask, &window_attr);
    
    m_pixel_buffer = XCreatePixmap(m_display, m_window, 
        m_width, m_height, m_depth);
    m_gc = XCreateGC(m_display, m_pixel_buffer, 0, 0);
    
    XSelectInput (m_display, m_window,
        ExposureMask | KeyPressMask | StructureNotifyMask | 
        ButtonPress | ButtonReleaseMask | PointerMotionMask);
     
    auto im = XOpenIM(m_display, nullptr, nullptr, nullptr);
    m_input_context = XCreateIC(im, XNInputStyle, 
        XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, m_window,
        NULL);
    
    load_font(font_name.c_str(), font_size);
    XMapWindow(m_display, m_window);
}

void XLibOutput::load_font(const std::string &&name, int size)
{
    m_font = XftFontOpenName(m_display, m_screen,  
        (name + ":size=" + std::to_string(size) + ":antialias=true").c_str());
    if (!m_font)
    {
        std::cerr << "terminal: xft: Couln't load font\n";
        return;
    }

    // NOTE: We're assuming monospaced fonts for now
    m_font_width = m_font->max_advance_width;
    m_font_height = m_font->height;
    
    // Helper lambda for loading Xft colors
    auto load_color = [&](XftColor &color, const std::string &hex)
    {
        auto str = "#" + hex;
        if (hex.length() > 6)
            str = "#" + hex.substr(2, hex.length() - 2);

        if (!XftColorAllocName(m_display, m_visual, m_color_map, str.c_str(), &color))
        {
            std::cerr << "terminal: xft: Could not allocate font color: " << str << "\n";
            return;
        }
    };

    // Load the full 8 color pallet
    load_color(m_text_pallet.default_background, ColorPalette::DefaultBackground);
    load_color(m_text_pallet.default_foreground, ColorPalette::DefaultForeground);
    load_color(m_text_pallet.black, ColorPalette::Black);
    load_color(m_text_pallet.red, ColorPalette::Red);
    load_color(m_text_pallet.green, ColorPalette::Green);
    load_color(m_text_pallet.yellow, ColorPalette::Yellow);
    load_color(m_text_pallet.blue, ColorPalette::Blue);
    load_color(m_text_pallet.magenta, ColorPalette::Magenta);
    load_color(m_text_pallet.cyan, ColorPalette::Cyan);
    load_color(m_text_pallet.white, ColorPalette::White);
    
    m_draw = XftDrawCreate(m_display, m_pixel_buffer, m_visual, m_color_map);
    if (!m_draw)
    {
        std::cerr << "terminal: xft: Could not create draw\n";
        return;
    }
}

CursorPosition XLibOutput::cursor_position_from_pixels(int x, int y)
{
    int curr_y = 0;
    for (int row = 0; row < rows(); row++)
    {
        if (y >= curr_y && y <= curr_y + m_font->height)
        {
            // We've found the row, now find the coloumn
            int curr_x = 0;
            
            for (int coloumn = 0; coloumn < columns(); coloumn++)
            {
                auto rune = rune_at(CursorPosition(coloumn, row));
                
                XGlyphInfo extents;
                XftTextExtentsUtf8(m_display, m_font, 
                    (const FcChar8 *)&rune.value, 1, &extents);
                
                curr_x += extents.xOff;
                if (x >= curr_x && x <= curr_x + extents.xOff)
                    return CursorPosition(coloumn, row);
            }
            
            // If no coloumn was found, then make it the end of the line
            return CursorPosition(columns() - 1, row);
        }

        curr_y += m_font->height;
    }

    // If no row was found, make it the last line
    return CursorPosition(0, rows());
}

std::string XLibOutput::update()
{
    XEvent event;
    while (XNextEvent(m_display, &event) == 0)
    {
        switch (event.type)
        {
            case Expose:
                redraw_all();
                break;
            
            case KeyPress:
                return decode_key_press(&event.xkey);
            
            case ButtonPress:
                switch (event.xbutton.button)
                {
                    case Button1:
                        // TODO: Start a new selection
                        break;
                    
                    case Button4:
                        // TODO: Scrollback up
                        break;

                    case Button5:
                        // TODO: Scrollback down
                        break;
                }
                break;

            case ButtonRelease:
                switch (event.xbutton.button)
                {
                    case Button1:
                        // TODO: End selection
                        break;
                }
                break;
            
            case MotionNotify:
            {
                auto x = event.xmotion.x;
                auto y = event.xmotion.y;
                m_mouse_pos = cursor_position_from_pixels(x, y);
                
                // TODO: Update selection
                break;
            }
             
            case ConfigureNotify:
                if (m_width != event.xconfigure.width || 
                    m_height != event.xconfigure.height)
                {
                    m_width = event.xconfigure.width;
                    m_height = event.xconfigure.height;
                    auto rows = (m_height / m_font_height);
                    auto columns = (m_width / m_font_width);

                    if (on_resize)
                    {
                        // Tell the terminal program that we've resized
                        struct winsize size;
                        size.ws_row = rows;
                        size.ws_col = columns;
                        size.ws_xpixel = m_width;
                        size.ws_ypixel = m_height;

                        on_resize(size);
                        resize(rows, columns);
                    }

                    // Create a new back buffer with the new size
                    XFreePixmap(m_display, m_pixel_buffer);
                    m_pixel_buffer = XCreatePixmap(m_display, m_window, 
                        m_width, m_height, m_depth);
                    
                    // We need to tell Xft about the new buffer
                    XftDrawDestroy(m_draw);
                    m_draw = XftDrawCreate(m_display, m_pixel_buffer, 
                        m_visual, m_color_map);
                }
                break;
        }
        
        break;
    }
    
    return "";
}

void XLibOutput::redraw_all()
{
    auto color = TerminalColor(TerminalColor::DefaultForeground, TerminalColor::DefaultBackground);    
    
    XSetForeground(m_display, m_gc, color.background_int());
    XFillRectangle(m_display, m_pixel_buffer, m_gc,
        0, 0, m_width, m_height);

    for (int row = 0; row < rows(); row++)
    {
        for (int column = 0; column < columns(); column++)
        {
            auto pos = CursorPosition(column, row);
            if (!isspace(rune_at(pos).value))
                draw_rune(pos);
        }
    }
}

std::string XLibOutput::decode_key_press(XKeyEvent *key_event)
{
    char buf[64];
    KeySym ksym;
    Status status;
    
    auto len = XmbLookupString(m_input_context, key_event, 
        buf, sizeof(buf), &ksym, &status);
    
    switch (ksym)
    {
        case XK_Up: return "\033[A";
        case XK_Down: return "\033[B";
        case XK_Right: return "\033[C";
        case XK_Left: return "\033[D";

        case XK_Home: return "\033[H";
        case XK_End: return "\033[F";

        case XK_BackSpace: return "\b";
    }
    
    if (len == 0)
        return "";
        
    return std::string(buf, len);
}

void XLibOutput::draw_scroll(int begin, int end, int by)
{
    auto by_pixels = by * m_font_height;
    auto top_of_buffer = begin * m_font_height + by_pixels;
    auto bottom_of_buffer = (end + 1) * m_font_height;
    auto height_of_buffer = bottom_of_buffer - top_of_buffer;
    XCopyArea(m_display, m_pixel_buffer, m_pixel_buffer, m_gc, 
        0, top_of_buffer, m_width, height_of_buffer, 0, top_of_buffer - by_pixels);

    auto color = TerminalColor(TerminalColor::DefaultForeground, TerminalColor::DefaultBackground);
    if (by > 0)
    {
        // Down
        XSetForeground(m_display, m_gc, color.background_int());
        XFillRectangle(m_display, m_pixel_buffer, m_gc, 
            0, bottom_of_buffer - by_pixels, m_width, by_pixels);
    }
    else
    {
        // Up
        XSetForeground(m_display, m_gc, color.background_int());
        XFillRectangle(m_display, m_pixel_buffer, m_gc, 
            0, 0, m_width, top_of_buffer - by_pixels);
    }
}

XftColor &XLibOutput::text_color_from_terminal(TerminalColor color)
{
    switch (color.foreground())
    {
        case TerminalColor::DefaultBackground: return m_text_pallet.default_background;
        case TerminalColor::DefaultForeground: return m_text_pallet.default_foreground;

        case TerminalColor::Black: return m_text_pallet.black;
        case TerminalColor::Red: return m_text_pallet.red;
        case TerminalColor::Green: return m_text_pallet.green;
        case TerminalColor::Yellow: return m_text_pallet.yellow;
        case TerminalColor::Blue: return m_text_pallet.blue;
        case TerminalColor::Magenta: return m_text_pallet.magenta;
        case TerminalColor::Cyan: return m_text_pallet.cyan;
        case TerminalColor::White: return m_text_pallet.white;
    }
    
    return m_text_pallet.white;
}

void XLibOutput::draw_rune(const CursorPosition &pos)
{
    auto &rune = rune_at(pos);
    auto color = rune.color;

    auto c = rune.value;
    auto x = (pos.coloumn() + 1) * m_font_width;
    auto y = (pos.row() + 1) * m_font_height;
    XSetForeground(m_display, m_gc, color.background_int());
    XFillRectangle(m_display, m_pixel_buffer, m_gc,
        x, y - m_font_height + 4, m_font_width, m_font_height);

    if (!isspace(rune.value))
    {
        XftDrawStringUtf8(m_draw, &text_color_from_terminal(color),
            m_font, x, y,
            (const FcChar8 *)&c, 1);
    }
}

void XLibOutput::draw_cursor()
{
    auto &rune = rune_at(cursor());
    auto color = rune.color.inverted();

    auto c = rune.value;
    auto x = (cursor().coloumn() + 1) * m_font_width;
    auto y = (cursor().row() + 1) * m_font_height;
    XSetForeground(m_display, m_gc, color.background_int());
    XFillRectangle(m_display, m_pixel_buffer, m_gc,
        x, y - m_font_height + 4, m_font_width, m_font_height);

    if (!isspace(rune.value))
    {
        XftDrawStringUtf8(m_draw, &text_color_from_terminal(color),
            m_font, x, y,
            (const FcChar8 *)&c, 1);
    }
}

void XLibOutput::flush_display()
{
    XCopyArea(m_display, m_pixel_buffer, m_window, m_gc, 
        0, 0, m_width, m_height, 0, 0);
    XFlush(m_display);
}

int XLibOutput::input_file() const
{
    return XConnectionNumber(m_display);
}

XLibOutput::~XLibOutput()
{
    if (m_font)
        XftFontClose(m_display, m_font);

    if (m_pixel_buffer)
        XFreePixmap(m_display, m_pixel_buffer);
    
    if (m_draw)
        XftDrawDestroy(m_draw);
}
