#include "xclipboard.hpp"
#include <iostream>
#include <string>
#include <cstring>

XClipBoard::XClipBoard(Display *display, Window window)
    : m_display(display)
    , m_window(window)
{
    m_utf8_atom = XInternAtom(display, "text/plain;charset=utf-8", 1);
    m_clipboard_atom = XInternAtom(display, "CLIPBOARD", 0);
    m_xsel_data_atom = XInternAtom(display, "XSEL_DATA", 0);
}

std::string XClipBoard::get_utf8_prop(Atom prop)
{
    Atom da, incr, type;
    int di;
    unsigned long size, dul;
    unsigned char *prop_ret = NULL;

    // Dummy call to get type and size
    XGetWindowProperty(m_display, m_window, prop, 0, 0, False, AnyPropertyType,
                       &type, &di, &dul, &size, &prop_ret);
    XFree(prop_ret);

    incr = XInternAtom(m_display, "INCR", False);
    if (type == incr)
    {
        std::cerr << "Data too large and INCR mechanism not implemented\n";
        return "";
    }

    // Read the data in one go.
    XGetWindowProperty(m_display, m_window, prop, 0, size, False, AnyPropertyType,
                       &da, &di, &dul, &dul, &prop_ret);
    auto data = std::string((const char*)prop_ret);
    XFree(prop_ret);

    // Signal the selection owner that we have successfully read the data
    XDeleteProperty(m_display, m_window, prop);

    return data;
}

std::string XClipBoard::paste()
{
    if (m_do_i_own_clipboard)
        return m_my_clipboard_data;

    auto owner = XGetSelectionOwner(m_display, m_clipboard_atom);
    if (!owner)
    {
        std::cerr << "The clipboard has no owner\n";
        return "";
    }

    XConvertSelection(m_display, m_clipboard_atom, m_utf8_atom,
        m_xsel_data_atom, m_window, CurrentTime);

    XEvent event;
    for (;;)
    {
        XNextEvent(m_display, &event);
        switch (event.type)
        {
            case SelectionNotify:
            {
                auto sev = (XSelectionEvent*)&event.xselection;
                if (sev->property == None)
                {
                    std::cerr << "Conversion could not be performed\n";
                    return "";
                }

                return get_utf8_prop(m_xsel_data_atom);
            }
        }
    }

    return "";
}

void XClipBoard::copy(const std::string &str)
{
    // Claim clipboard ownership
    XSetSelectionOwner(m_display, m_clipboard_atom, m_window, CurrentTime);

    // Update our clipboard data
    m_my_clipboard_data = str;
    m_do_i_own_clipboard = true;
}

void XClipBoard::send_no(XSelectionRequestEvent *sev)
{
    // All of these should match the values of the request
    XSelectionEvent ssev;
    ssev.type = SelectionNotify;
    ssev.requestor = sev->requestor;
    ssev.selection = sev->selection;
    ssev.target = sev->target;
    ssev.property = None;  // signifies 'nope'
    ssev.time = sev->time;

    XSendEvent(m_display, sev->requestor, True, NoEventMask, (XEvent*)&ssev);
}

void XClipBoard::send_utf8(XSelectionRequestEvent *sev)
{
    XChangeProperty(m_display, sev->requestor,
        sev->property, m_utf8_atom, 8, PropModeReplace,
        (unsigned char*)m_my_clipboard_data.data(), m_my_clipboard_data.size());

    XSelectionEvent ssev;
    ssev.type = SelectionNotify;
    ssev.requestor = sev->requestor;
    ssev.selection = sev->selection;
    ssev.target = sev->target;
    ssev.property = sev->property;
    ssev.time = sev->time;
    XSendEvent(m_display, sev->requestor, True, NoEventMask, (XEvent*)&ssev);
}

void XClipBoard::update(XEvent &event)
{
    switch (event.type)
    {
        case SelectionClear:
        {
            m_my_clipboard_data.clear();
            m_do_i_own_clipboard = false;
            break;
        }
        case SelectionRequest:
        {
            auto sev = (XSelectionRequestEvent*)&event.xselectionrequest;

            // Property is set to None by "obsolete" clients.
            if (sev->target != m_utf8_atom || sev->property == None)
                send_no(sev);
            else
                send_utf8(sev);
            break;
        }
    }

}
