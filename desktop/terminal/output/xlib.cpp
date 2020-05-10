#include "xlib.hpp"
#include "../escapes.hpp"
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

    m_screen = DefaultScreen(m_display);
    m_visual = DefaultVisual(m_display, m_screen);
    m_color_map = DefaultColormap(m_display, m_screen);
    m_window = XCreateSimpleWindow(m_display, 
        RootWindow(m_display, m_screen), 
        10, 10, 100, 100, 1, 
        BlackPixel(m_display, m_screen), WhitePixel(m_display, m_screen));

    auto im = XOpenIM(m_display, nullptr, nullptr, nullptr);
    m_input_context = XCreateIC(im, XNInputStyle, 
        XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, m_window,
        NULL);
    
    load_font("DejaVu Sans Mono", 16);
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
    
    if (!XftColorAllocName(m_display, m_visual, m_color_map, "#FFFFFF", &m_color))
    {
        std::cerr << "terminal: xft: Could not allocate font\n";
        return;
    }
    
    m_draw = XftDrawCreate(m_display, m_window, m_visual, m_color_map);
    if (!m_draw)
    {
        std::cerr << "terminal: xft: Could not create draw\n";
        return;
    }

    XSelectInput(m_display, m_window, ExposureMask | KeyPressMask);
    XMapWindow(m_display, m_window);
}

std::string XLibOutput::update()
{
    XEvent event;
    XNextEvent(m_display, &event);
    
    switch (event.type)
    {
        case Expose:
            redraw_all();
            break;
        
        case KeyPress:
            return decode_key_press(&event.xkey);
    }
    
    return "";
}

void XLibOutput::redraw_all()
{
    auto gc = DefaultGC(m_display, m_screen);
    
    XWindowAttributes attr;
    XGetWindowAttributes(m_display, m_window, &attr);
    m_width = attr.width;
    m_height = attr.height;
    
    XSetForeground(m_display, gc, 0x000000);
    XFillRectangle(m_display, m_window, gc,
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

void XLibOutput::draw_window()
{
    XWindowAttributes attr;
    XGetWindowAttributes(m_display, m_window, &attr);
    m_width = attr.width;
    m_height = attr.height;

    draw_buffer();
    XFlush(m_display);
}

Line &XLibOutput::line_at(const CursorPosition &position)
{
    while (position.row() >= m_lines.size())
        m_lines.emplace_back();
    
    return m_lines[position.row()];
}

void XLibOutput::draw_buffer()
{
    if (!m_font || !m_draw)
    {
        std::cerr << "terminal: No font loaded\n";
        return;
    }
    
    auto gc = DefaultGC(m_display, m_screen);
    int y = m_font->height;
    for (int row = 0; row < m_lines.size(); row++)
    {
        auto &line = m_lines[row];
        if (line.is_dirty())
        {
            int x = m_font->max_advance_width;
            XSetForeground(m_display, gc, 0x000000);
            XFillRectangle(m_display, m_window,
                gc, 0, y - m_font->height + 4, m_width, m_font->height);
            
            for (int column = 0; column < line.data().length(); column++)
            {
                char c = line.data()[column];
                XftDrawStringUtf8(m_draw, &m_color, m_font, 
                    x, y, 
                    (const FcChar8 *)&c, 1);
                
                XGlyphInfo extents;
                XftTextExtentsUtf8(m_display, m_font, 
                    (const FcChar8 *)&c, 1, &extents);
                x += extents.xOff;

                if (m_cursor.row() == row && m_cursor.coloumn() - 1 == column)
                {
                    XSetForeground(m_display, gc, 0xFFFFFF);
                    XFillRectangle(m_display, m_window, gc, 
                        x, y - m_font->height + m_font->descent, 10, 
                        m_font->height - m_font->ascent + m_font->descent);
                }
            }
            
            line.unmark_dirty();
        }
        
        y += m_font->height;
    }
}

void XLibOutput::write(std::string_view buff)
{
    for (int i = 0; i < buff.length(); i++)
    {
        char c = buff[i];
        auto escape_sequence = EscapesSequence::parse(
            std::string_view(buff.data() + i, buff.length() - i));
        
        if (!escape_sequence)
        {
            if (c == '\n')
            {
                m_cursor.move_by(0, 1);
                m_cursor.move_to_begging_of_line();
            }
            else
            {
                line_at(m_cursor).set(m_cursor.coloumn(), c);
                m_cursor.move_by(1, 0);
            }
            
            continue;
        }
        
        std::cout << "Escape: " << *escape_sequence << "\n";
        switch (escape_sequence->type())
        {
            case EscapesSequence::Cursor:
            {
                auto cursor = static_cast<EscapeCursor&>(*escape_sequence);
                switch (cursor.direction())
                {
                    case EscapeCursor::Right:
                        m_cursor.move_by(-1, 0);
                        line_at(m_cursor).mark_dirty();
                        break;
                    
                    case EscapeCursor::TopLeft:
                        m_cursor.move_to(0, 0);
                        break;
                    
                    default:
                        break;
                }
                break;
            }
            
            case EscapesSequence::Insert:
            {
                auto insert = static_cast<EscapeInsert&>(*escape_sequence);
                if (i + escape_sequence->char_count() + insert.count() < buff.length())
                {
                    auto str = std::string_view(
                        buff.data() + i + escape_sequence->char_count() + 1, insert.count());

                    for (char c : str)
                        line_at(m_cursor).insert(m_cursor.coloumn(), c);
                    i += insert.count();
                    m_cursor.move_by(insert.count(), 0);
                }
                break;
            }
            
            case EscapesSequence::Delete:
            {
                auto del = static_cast<EscapeInsert&>(*escape_sequence);
                line_at(m_cursor).erase(m_cursor.coloumn() - del.count() + 1, del.count());
                break;
            }
            
            case EscapesSequence::Clear:
            {
                auto clear = static_cast<EscapeClear&>(*escape_sequence);
                
                switch (clear.mode())
                {
                    case EscapeClear::CursorToLineEnd: 
                    {
                        auto index = m_cursor.coloumn();
                        auto &line = m_lines[m_cursor.row()];
                        while (index < line.data().length())
                        {
                            line.set(index, ' ');
                            index += 1;
                        }
                        break;
                    }
                    
                    case EscapeClear::Screen:
                        m_lines.clear();
                        redraw_all();
                        break;
                    
                    default:
                        break;
                }
                
                break;
            }
            
            default:
                break;
        }
        
        i += escape_sequence->char_count();
    }

    draw_window();
}

int XLibOutput::input_file() const
{
    return XConnectionNumber(m_display);
}

XLibOutput::~XLibOutput()
{
    if (m_font)
        XftFontClose(m_display, m_font);
    
    if (m_draw)
        XftDrawDestroy(m_draw);
}
