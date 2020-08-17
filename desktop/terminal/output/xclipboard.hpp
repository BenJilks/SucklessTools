#pragma once
#include <X11/Xlib.h>
#include <string>

class XClipBoard
{
public:
    XClipBoard(Display *display, Window window);

    std::string paste();

private:
    Display *m_display;
    Window m_window;

    Atom m_targets_atom;
    Atom m_text_atom;
    Atom m_utf8_atom;
    Atom m_clipboard_atom;
    Atom m_xsel_data_atom;

};
