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
    for (int row = 0; row < m_rows; row++)
    {
        if (y >= curr_y && y <= curr_y + m_font->height)
        {
            // We've found the row, now find the coloumn
            auto &line = line_at(CursorPosition(0, row));
            int curr_x = 0;
            
            for (int coloumn = 0; coloumn < (int)line.length(); coloumn++)
            {
                auto rune = line[coloumn];
                
                XGlyphInfo extents;
                XftTextExtentsUtf8(m_display, m_font, 
                    (const FcChar8 *)&rune.value, 1, &extents);
                
                curr_x += extents.xOff;
                if (x >= curr_x && x <= curr_x + extents.xOff)
                    return CursorPosition(coloumn, row);
            }
            
            // If no coloumn was found, then make it the end of the line
            return CursorPosition(line.length(), row);
        }

        curr_y += m_font->height;
    }

    // If no row was found, make it the last line
    return CursorPosition(0, m_lines.size());
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
                        // Start a new selection
                        m_selection_start = m_mouse_pos;
                        m_in_selection = true;
                        if (m_has_selection)
                        {
                            // If there already is one, remove it so that clicking 
                            // without moving the mouse will unselect all
                            m_has_selection = false;
                            draw_window();
                        }
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
                        m_in_selection = false;
                        break;
                }
                break;
            
            case MotionNotify:
            {
                auto x = event.xmotion.x;
                auto y = event.xmotion.y;
                m_mouse_pos = cursor_position_from_pixels(x, y);
                
                if (m_in_selection)
                {
                    // If we're in a selction, update it
                    m_has_selection = true;
                    m_selection_end = m_mouse_pos;
                    line_at(m_selection_end).mark_dirty();
                    draw_window();
                }
                break;
            }
             
            case ConfigureNotify:
                if (m_width != event.xconfigure.width || 
                    m_height != event.xconfigure.height)
                {
                    m_width = event.xconfigure.width;
                    m_height = event.xconfigure.height;
                    auto rows = m_height / m_font->height;
                    auto columns = m_width / m_font->max_advance_width;

                    if (on_resize)
                    {
                        // Tell the terminal program that we've resized
                        struct winsize size;
                        size.ws_row = rows;
                        size.ws_col = columns;
                        size.ws_xpixel = m_width;
                        size.ws_ypixel = m_height;

                        m_cursor.move_to_begging_of_line();
                        line_at(m_cursor).clear();

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

    for (auto &line : m_lines)
        line.mark_dirty();

    draw_window();
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
    auto by_pixels = by * m_font->height;
    XCopyArea(m_display, m_pixel_buffer, m_pixel_buffer, m_gc, 
        0, 0, m_width, m_height, 0, -by_pixels);

    auto color = TerminalColor(TerminalColor::DefaultForeground, TerminalColor::DefaultBackground);
    if (by > 0)
    {
        // Down
        XSetForeground(m_display, m_gc, color.background_int());
        XFillRectangle(m_display, m_pixel_buffer, m_gc, 
            0, ((m_rows - 1) * m_font->height) - by_pixels, m_width, by_pixels);
    }
    else
    {
        // Up
        XSetForeground(m_display, m_gc, color.background_int());
        XFillRectangle(m_display, m_pixel_buffer, m_gc, 
            0, 0, m_width, -by_pixels);
    }

    draw_window();
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

void XLibOutput::draw_window()
{
    if (!m_font || !m_draw)
    {
        std::cerr << "terminal: No font loaded\n";
        return;
    }
    
    for (int row = 0; row < m_rows; row++)
    {
        auto &line = m_lines[row];
        auto color = TerminalColor(TerminalColor::DefaultForeground, TerminalColor::DefaultBackground);
        
        bool in_selection = line_in_selection(row);
        if (line.is_dirty() || line.was_in_selection() || in_selection)
        {
            int selection_start, selection_end;
            int background_color = color.background_int();
            line.unmark_in_selection();
            if (in_selection)
            {
                line.mark_in_selection();
                selection_start = line_selection_start(row);
                selection_end = line_selection_end(row);
                if (selection_start == 0 && selection_end == (int)line.length())
                    background_color = color.foreground_int();
            }

            auto y = (row + 1) * m_font_height;
            XSetForeground(m_display, m_gc, background_color);
            XFillRectangle(m_display, m_pixel_buffer,
                m_gc, 0, y - m_font->height + 4, m_width, m_font->height);
            
            for (int column = 0; column < (int)line.length(); column++)
            {
                const auto &rune = line[column];
                color = rune.color;
                
                auto effective_color = color;
                if (in_selection)
                {
                    if (column >= selection_start && column <= selection_end)
                        effective_color = color.inverted();
                }
                
                if (m_cursor.row() == row && m_cursor.coloumn() == column)
                    effective_color = color.inverted();
                
                auto c = rune.value;
                auto x = (column + 1) * m_font_width;
                XSetForeground(m_display, m_gc, effective_color.background_int());
                XFillRectangle(m_display, m_pixel_buffer, m_gc, 
                    x, y - m_font_height + 4, m_font_width, m_font_height);
                
                XftDrawStringUtf8(m_draw, &text_color_from_terminal(effective_color), 
                    m_font, x, y, 
                    (const FcChar8 *)&c, 1);
            }
            
            if (m_cursor.row() == row && m_cursor.coloumn() == (int)line.length())
            {
                XSetForeground(m_display, m_gc, color.foreground_int());
                XFillRectangle(m_display, m_pixel_buffer, m_gc, 
                    (line.length() + 1) * m_font_width, y - m_font->height + 4,
                    m_font->max_advance_width, m_font->height);
            }
            
            line.unmark_dirty();
        }
    }

    swap_buffers();
}

void XLibOutput::swap_buffers()
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
