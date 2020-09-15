#pragma once
#include <X11/Xlib.h>
#include <string>

class XClipBoard
{
public:
    XClipBoard(Display *display, Window window);

    std::string paste();
    void copy(const std::string&);

    void update(XEvent &event);

private:
    Display *m_display;
    Window m_window;

    Atom m_utf8_atom;
    Atom m_clipboard_atom;
    Atom m_xsel_data_atom;
    std::string m_my_clipboard_data;
    bool m_do_i_own_clipboard { false };

    std::string get_utf8_prop(Atom prop);
    void send_no(XSelectionRequestEvent*);
    void send_utf8(XSelectionRequestEvent*);

};
