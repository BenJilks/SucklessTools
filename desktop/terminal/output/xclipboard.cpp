#include "xclipboard.hpp"
#include <iostream>

XClipBoard::XClipBoard(Display *display, Window window)
    : m_display(display)
    , m_window(window)
{
    m_targets_atom = XInternAtom(display, "TARGETS", 0);
    m_text_atom = XInternAtom(display, "TEXT", 0);
    m_utf8_atom = XInternAtom(display, "UTF8_STRING", 1);
    m_clipboard_atom = XInternAtom(display, "CLIPBOARD", 0);
    m_xsel_data_atom = XInternAtom(display, "XSEL_DATA", 0);
}

std::string XClipBoard::paste()
{
    Atom da, incr, type;
    int di;
    unsigned long size, dul;
    unsigned char *prop_ret = NULL;

    /* Dummy call to get type and size. */
    XGetWindowProperty(m_display, m_window, m_clipboard_atom, 0, 0, False, AnyPropertyType,
                       &type, &di, &dul, &size, &prop_ret);
    XFree(prop_ret);

    std::cout << "Size: " << size << "\n";

    incr = XInternAtom(m_display, "INCR", False);
    if (type == incr)
    {
        printf("Data too large and INCR mechanism not implemented\n");
        return "";
    }

    /* Read the data in one go. */
    XGetWindowProperty(m_display, m_window, m_clipboard_atom, 0, size, False, AnyPropertyType,
                       &da, &di, &dul, &dul, &prop_ret);
    std::cout << prop_ret << "\n";
    XFree(prop_ret);

    /* Signal the selection owner that we have successfully read the
     * data. */
    XDeleteProperty(m_display, m_window, m_clipboard_atom);

    return "";
}
