#include "xlib.hpp"
#include "../escapes.hpp"
#include <iostream>
#include <sstream>
#include <cstring>

XLibOutput::XLibOutput()
    : m_cursor(0)
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
            draw_window();
            break;
        
        case KeyPress:
            return decode_key_press(&event.xkey);
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

void XLibOutput::draw_window() const
{
    auto gc = DefaultGC(m_display, m_screen);

    XWindowAttributes attr;
    XGetWindowAttributes(m_display, m_window, &attr);
    
    XSetForeground(m_display, gc, 0x000000);
    XFillRectangle(m_display, m_window,
        gc, 0, 0, attr.width, attr.height);
    
    draw_buffer();
    XFlush(m_display);
}

std::vector<std::string> XLibOutput::lines() const
{
    std::vector<std::string> out;
    std::stringstream stream(m_buffer);
    std::string line;
    
    while (std::getline(stream, line, '\n'))
        out.push_back(line);
    return out;
}

void XLibOutput::draw_buffer() const
{
    if (!m_font || !m_draw)
    {
        std::cerr << "terminal: No font loaded\n";
        return;
    }
    
    auto gc = DefaultGC(m_display, m_screen);
    auto buffer_lines = lines();
    int curr_pos = 0;
    for (int i = 0; i < buffer_lines.size(); i++)
    {
        auto line = buffer_lines[i];
        int y = m_font->height * (i + 1);
        int x = m_font->max_advance_width;
        
        for (char c : line)
        {
            XftDrawStringUtf8(m_draw, &m_color, m_font, 
                x, y, 
                (const FcChar8 *)&c, 1);
            
            XGlyphInfo extents;
            XftTextExtentsUtf8(m_display, m_font, 
                (const FcChar8 *)&c, 1, &extents);
            x += extents.xOff;

            curr_pos += 1;
            if (m_cursor == curr_pos)
            {
                XSetForeground(m_display, gc, 0xFFFFFF);
                XFillRectangle(m_display, m_window, gc, 
                    x, y - m_font->height + m_font->descent, 10, m_font->height - m_font->ascent + m_font->descent);
            }
        }
        
        curr_pos += 1; // Add the newline
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
            if ((c >= 33 && c <= 126) || std::isspace(c))
            {
                if (m_cursor >= m_buffer.length())
                    m_buffer += c;
                else
                    m_buffer[m_cursor] = c;
                
                m_cursor += 1;
                continue;
            }
            std::cout << "Uknown char: " << (int)c << "\n";
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
                        m_cursor -= 1;
                        break;
                    
                    case EscapeCursor::TopLeft:
                        std::cout << "FIXME: xlib: write: Do TopLeft move for real\n";
                        m_cursor = 0;
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

                    m_buffer.insert(m_cursor, str);
                    i += insert.count();
                    m_cursor += insert.count();
                }
                break;
            }
            
            case EscapesSequence::Delete:
            {
                auto del = static_cast<EscapeInsert&>(*escape_sequence);
                m_buffer.erase(m_cursor - del.count() + 1, del.count());
                break;
            }
            
            case EscapesSequence::Clear:
            {
                auto clear = static_cast<EscapeClear&>(*escape_sequence);
                
                switch (clear.mode())
                {
                    case EscapeClear::CursorToLineEnd: 
                    {
                        int index = m_cursor;
                        while (index < m_buffer.length())
                        {
                            char c = m_buffer[index];
                            if (c == '\n')
                                break;
                            
                            m_buffer[index] = ' ';
                            index += 1;
                        }
                        break;
                    }
                    
                    case EscapeClear::Screen:
                        m_buffer.clear();
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
