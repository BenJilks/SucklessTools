#include "xlib.hpp"
#include <iostream>
#include <sstream>
#include <cstring>

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
    m_visual = DefaultVisual(m_display, m_screen);
    m_color_map = DefaultColormap(m_display, m_screen);
    m_window = XCreateSimpleWindow(m_display, 
        RootWindow(m_display, m_screen), 
        10, 10, m_width, m_height, 1, 
        BlackPixel(m_display, m_screen), WhitePixel(m_display, m_screen));
    
    m_pixel_buffer = XCreatePixmap(m_display, m_window, 
        m_width, m_height, XDefaultDepth(m_display, m_screen));
    
    XSelectInput (m_display, m_window,
        ExposureMask | KeyPressMask | StructureNotifyMask | 
        ButtonPress | ButtonReleaseMask | PointerMotionMask);
    
    auto im = XOpenIM(m_display, nullptr, nullptr, nullptr);
    m_input_context = XCreateIC(im, XNInputStyle, 
        XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, m_window,
        NULL);
    
    load_font("DejaVu Sans Mono", 16);
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
    
    auto load_color = [&](XftColor &color, const std::string &hex)
    {
        if (!XftColorAllocName(m_display, m_visual, m_color_map, ("#" + hex).c_str(), &color))
        {
            std::cerr << "terminal: xft: Could not allocate font\n";
            return;
        }
    };

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
    for (int row = m_curr_frame_index; row < m_curr_frame_index + m_rows; row++)
    {
        if (y >= curr_y && y <= curr_y + m_font->height)
        {
            const auto &line = line_at(CursorPosition(0, row));
            int curr_x = 0;
            
            for (int coloumn = 0; coloumn < line.data().length(); coloumn++)
            {
                int c = line.data()[coloumn];
                
                XGlyphInfo extents;
                XftTextExtentsUtf8(m_display, m_font, 
                    (const FcChar8 *)&c, 1, &extents);
                
                if (x >= curr_x && x <= curr_x + extents.xOff)
                    return CursorPosition(coloumn, row);
                    
                curr_x += extents.xOff;
            }
            
            return CursorPosition(line.data().length(), row);
        }

        curr_y += m_font->height;
    }
    
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
                        m_selection_start = m_mouse_pos;
                        m_in_selection = true;
                        if (m_has_selection)
                        {
                            m_has_selection = false;
                            draw_window();
                        }
                        break;
                    
                    case Button4:
                        scroll(-1);
                        break;

                    case Button5:
                        scroll(1);
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
                    m_rows = m_height / m_font->height;

                    XFreePixmap(m_display, m_pixel_buffer);
                    m_pixel_buffer = XCreatePixmap(m_display, m_window, 
                        m_width, m_height, XDefaultDepth(m_display, m_screen));
                    
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
    auto gc = DefaultGC(m_display, m_screen);
    
    XSetForeground(m_display, gc, 0x000000);
    XFillRectangle(m_display, m_pixel_buffer, gc,
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

void XLibOutput::scroll(int by)
{
    if (m_curr_frame_index + by < 0)
        by = -m_curr_frame_index;
    
    auto by_pixels = by * m_font->height;
    auto gc = DefaultGC(m_display, m_screen);
    
    XCopyArea(m_display, m_pixel_buffer, m_pixel_buffer, gc, 
        0, 0, m_width, m_height, 0, -by_pixels);

    if (by > 0)
    {
        // Down
        XSetForeground(m_display, gc, 0x000000);
        XFillRectangle(m_display, m_pixel_buffer, gc, 
            0, m_height - by_pixels, m_width, by_pixels);
        
        for (int i = m_curr_frame_index - by; i < m_curr_frame_index; i++)
        {
            if (i >= 0)
                m_lines[i].mark_dirty();
        }
    }
    else
    {
        // Up
        XSetForeground(m_display, gc, 0x000000);
        XFillRectangle(m_display, m_pixel_buffer, gc, 
            0, 0, m_width, -by_pixels);

        for (int i = m_curr_frame_index + m_rows; i < m_curr_frame_index + m_rows - (by - 1); i++)
        {
            if (i < m_lines.size())
                m_lines[i].mark_dirty();
        }
    }

    m_curr_frame_index += by;
    draw_window();
}

int XLibOutput::line_selection_start(int row)
{
    auto start = m_selection_start;
    auto end = m_selection_end;
    if (end.row() < start.row())
        std::swap(start, end);
    
    if (row == start.row() && row == end.row())
        return std::min(start.coloumn(), end.coloumn());
    
    if (row == start.row())
        return start.coloumn();
    
    return 0;
}

int XLibOutput::line_selection_end(int row)
{
    auto start = m_selection_start;
    auto end = m_selection_end;
    if (end.row() < start.row())
        std::swap(start, end);
    
    if (row == start.row() && row == end.row())
        return std::max(start.coloumn(), end.coloumn());
    
    if (row == end.row())
        return end.coloumn();
    
    return line_at(CursorPosition(0, row)).data().length();
}

XftColor &XLibOutput::text_color_from_terminal(TerminalColor color)
{
    switch (color.foreground())
    {
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
    
    auto gc = DefaultGC(m_display, m_screen);
    int y = m_font->height;
    for (int row = m_curr_frame_index; row < m_curr_frame_index + m_rows; row++)
    {
        if (row >= m_lines.size())
            break;
        auto &line = m_lines[row];
        auto color = TerminalColor(TerminalColor::White, TerminalColor::Black);
        
        bool in_selection = line_in_selection(row);
        if (line.is_dirty() || line.was_in_selection() || in_selection)
        {
            int selection_start, selection_end;
            int background_color = 0x000000;
            line.unmark_in_selection();
            if (in_selection)
            {
                line.mark_in_selection();
                selection_start = line_selection_start(row);
                selection_end = line_selection_end(row);
                if (selection_start == 0 && selection_end == line.data().length())
                    background_color = 0xFFFFFF;
            }

            int x = m_font->max_advance_width;
            XSetForeground(m_display, gc, background_color);
            XFillRectangle(m_display, m_pixel_buffer,
                gc, 0, y - m_font->height + 4, m_width, m_font->height);
            
            for (int column = 0; column < line.data().length(); column++)
            {
                const auto *attr = line.curr_attribute(column);
                if (attr)
                    color = attr->color();
                
                static auto default_color = TerminalColor(TerminalColor::Black, TerminalColor::White);
                static auto default_select = TerminalColor(TerminalColor::White, TerminalColor::Black);
                if (in_selection)
                {
                    if (column >= selection_start && column <= selection_end)
                        color = attr ? attr->inverted_color() : default_color;
                    else
                        color = attr ? attr->color() : default_select;
                }
                
                char c = line.data()[column];
                XGlyphInfo extents;
                XftTextExtentsUtf8(m_display, m_font, 
                    (const FcChar8 *)&c, 1, &extents);
                
                XSetForeground(m_display, gc, color.background_int());
                XFillRectangle(m_display, m_pixel_buffer, gc, 
                    x, y - m_font->height + 4, extents.xOff, m_font->height);
                
                XftDrawStringUtf8(m_draw, &text_color_from_terminal(color), 
                    m_font, x, y, 
                    (const FcChar8 *)&c, 1);
                
                x += extents.xOff;

                if (m_cursor.row() == row && m_cursor.coloumn() - 1 == column)
                {
                    XSetForeground(m_display, gc, 0xFFFFFF);
                    XFillRectangle(m_display, m_pixel_buffer, gc, 
                        x, y - m_font->height + m_font->descent, 10, 
                        m_font->height - m_font->ascent + m_font->descent);
                }
            }
            
            line.unmark_dirty();
        }
        
        y += m_font->height;
    }

    swap_buffers();
}

void XLibOutput::swap_buffers()
{
    auto gc = DefaultGC(m_display, m_screen);    
    XCopyArea(m_display, m_pixel_buffer, m_window, gc, 
        0, 0, m_width, m_height, 0, 0);
    XFlush(m_display);
}
 
bool XLibOutput::line_in_selection(int row)
{
    if (!m_has_selection)
        return false;
    
    auto start = m_selection_start.row();
    auto end = m_selection_end.row();
    if (end < start)
        std::swap(start, end);
    
    return row >= start && row <= end;
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
