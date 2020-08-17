#include "xlib.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <cmath>

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

    m_wm_delete_message = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_display, m_window, &m_wm_delete_message, 1);
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
    return CursorPosition(
        roundf(x / m_font_width),
        roundf(y / m_font_height));
}

void XLibOutput::input(const std::string &msg)
{
    m_input_buffer += msg;
}

std::string XLibOutput::update()
{
    if (!m_input_buffer.empty())
    {
        auto msg = m_input_buffer;
        m_input_buffer.clear();

        return msg;
    }

    XEvent event;
    while (XNextEvent(m_display, &event) == 0)
    {
        switch (event.type)
        {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == m_wm_delete_message)
                    set_should_close(true);
                break;

            case Expose:
                redraw_all();
                break;
            
            case KeyPress:
                return decode_key_press(&event.xkey);

            case KeyRelease:
                return decode_key_release(&event.xkey);
            
            case ButtonPress:
                switch (event.xbutton.button)
                {
                    case Button1:
                        for_rune_in_selection([this](const CursorPosition &pos)
                        {
                            draw_rune(pos);
                        });
                        m_selection_start = m_mouse_pos;
                        m_selection_end = m_mouse_pos;
                        m_in_selection = true;
                        flush_display();
                        break;
                    
                    case Button4:
                        if (-m_scroll_offset < buffer().scroll_back() - 1)
                        {
                            m_scroll_offset -= 1;
                            draw_scroll(0, rows(), -1);
                            draw_row(0, true);
                            flush_display();
                        }
                        break;

                    case Button5:
                        if (m_scroll_offset < 0)
                        {
                            m_scroll_offset += 1;
                            draw_scroll(0, rows(), 1);
                            draw_row(rows() - 1, true);
                            flush_display();
                        }
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
                    draw_update_selection(m_mouse_pos);
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

                    // Noop, so don't bother resizing anything
                    if (rows == this->rows() && columns == this->columns())
                        break;

                    if (on_resize)
                    {
                        // Tell the terminal program that we've resized
                        struct winsize size;
                        size.ws_row = rows;
                        size.ws_col = columns;
                        size.ws_xpixel = m_width;
                        size.ws_ypixel = m_height;

                        resize(rows, columns);
                        on_resize(size);
                    }

                    // Create a new back buffer with the new size
                    XFreePixmap(m_display, m_pixel_buffer);
                    m_pixel_buffer = XCreatePixmap(m_display, m_window, 
                        m_width + m_font_width, m_height + m_font_height, m_depth);
                    
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

template<typename CallbackFunc>
void XLibOutput::for_rune_in_selection(CallbackFunc callback)
{
    auto start = m_selection_start;
    auto end = m_selection_end;
    if (start.row() > end.row())
        std::swap(start, end);

    for (int row = start.row(); row <= end.row(); row++)
    {
        auto start_column = (row == start.row() ? start.coloumn() : 0);
        auto end_column = (row == end.row() ? end.coloumn() : columns());
        if (start_column > end_column)
            std::swap(start_column, end_column);

        for (int column = start_column; column < end_column; column++)
            callback(CursorPosition(column, row));
    }
}

void XLibOutput::draw_update_selection(const CursorPosition &new_end_pos)
{
    for_rune_in_selection([this](const CursorPosition &pos)
    {
        draw_rune(pos);
    });

    m_selection_end = new_end_pos;
    for_rune_in_selection([this](const CursorPosition &pos)
    {
        draw_rune(pos, true);
    });
    flush_display();
}

void XLibOutput::draw_row(int row, bool refresh)
{
    for (int column = 0; column < columns(); column++)
    {
        auto pos = CursorPosition(column, row);
        if (!isspace(buffer().rune_at_scroll_offset(pos, m_scroll_offset).value) || refresh)
            draw_rune(pos);
    }
}

void XLibOutput::redraw_all()
{
    auto color = TerminalColor(TerminalColor::DefaultForeground, TerminalColor::DefaultBackground);    
    
    XSetForeground(m_display, m_gc, color.background_int());
    XFillRectangle(m_display, m_pixel_buffer, m_gc,
        0, 0, m_width, m_height);

    for (int row = 0; row < rows(); row++)
        draw_row(row);

    draw_rune(cursor(), true);
    flush_display();
}

std::string XLibOutput::decode_key_release(XKeyEvent *key_event)
{
    char buf[64];
    KeySym ksym;
    Status status;

    XmbLookupString(m_input_context, key_event,
        buf, sizeof(buf), &ksym, &status);

    if (application_keys_mode())
    {
        switch (ksym)
        {
            case XK_Up: return "\033A";
            case XK_Down: return "\033B";
            case XK_Right: return "\033C";
            case XK_Left: return "\033D";
            default: break;
        }
    }

    return "";
}

std::string XLibOutput::decode_key_press(XKeyEvent *key_event)
{
    char buf[64];
    KeySym ksym;
    Status status;
    
    auto len = XmbLookupString(m_input_context, key_event, 
        buf, sizeof(buf), &ksym, &status);
    
    if (application_keys_mode())
    {
        // Special application keys
        switch (ksym)
        {
            case XK_Up: return "\033OA";
            case XK_Down: return "\033OB";
            case XK_Right: return "\033OC";
            case XK_Left: return "\033OD";
            default: break;
        }
    }

    switch (ksym)
    {
        case XK_Up: return "\033[A";
        case XK_Down: return "\033[B";
        case XK_Right: return "\033[C";
        case XK_Left: return "\033[D";

        case XK_Home: return "\033[H";
        case XK_End: return "\033[F";
        case XK_Page_Up: return "\033[5~";
        case XK_Page_Down: return "\033[6~";

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

    auto color = TerminalColor(TerminalColor::DefaultForeground, TerminalColor::DefaultBackground);
    if (by > 0)
    {
        // Down
        XCopyArea(m_display, m_pixel_buffer, m_pixel_buffer, m_gc,
            0, top_of_buffer, m_width, height_of_buffer, 0, top_of_buffer - by_pixels);

        XSetForeground(m_display, m_gc, color.background_int());
        XFillRectangle(m_display, m_pixel_buffer, m_gc,
            0, bottom_of_buffer - by_pixels, m_width, by_pixels);
        for (int i = end - by; i < end; i++)
            draw_row(i, true);
    }
    else
    {
        // Up
        XCopyArea(m_display, m_pixel_buffer, m_pixel_buffer, m_gc,
            0, top_of_buffer, m_width, height_of_buffer - m_font_height, 0, top_of_buffer - by_pixels);

        XSetForeground(m_display, m_gc, color.background_int());
        XFillRectangle(m_display, m_pixel_buffer, m_gc,
            0, 0, m_width, top_of_buffer - by_pixels + m_font_height);
        for (int i = begin; i < -by; i++)
            draw_row(i, true);
    }

    m_selection_start.move_by(0, -by);
    m_selection_end.move_by(0, -by);
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

void XLibOutput::draw_rune(const CursorPosition &pos, bool selected)
{
    auto &rune = buffer().rune_at_scroll_offset(pos, m_scroll_offset);
    auto color = rune.attribute.color();
    if (selected)
        color = color.inverted();

    auto c = rune.value;
    auto x = (pos.coloumn() + 1) * m_font_width;
    auto y = (pos.row() + 1) * m_font_height;
    XSetForeground(m_display, m_gc, color.background_int());
    XFillRectangle(m_display, m_pixel_buffer, m_gc,
        x, y - m_font_height + 4, m_font_width, m_font_height);

    if (!isspace(rune.value))
    {
        auto glyph = XftCharIndex(m_display, m_font, c);
        XRectangle rect = { 0, 0, (uint16_t)(m_font_width * 2), (uint16_t)(m_font_height) };

        XftDrawSetClipRectangles(m_draw, x - m_font_width, y - m_font_height, &rect, 1);
        XftDrawGlyphs(m_draw, &text_color_from_terminal(color),
            m_font, x, y, &glyph, 1);

        // Reset clip
        XftDrawSetClip(m_draw, 0);
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
