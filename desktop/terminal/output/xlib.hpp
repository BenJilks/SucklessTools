#pragma once
#include "output.hpp"
#include "xclipboard.hpp"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xdbe.h>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

struct XftPallet
{
    XftColor default_background;
    XftColor default_foreground;
    
    XftColor black;
    XftColor red;
    XftColor green;
    XftColor yellow;
    XftColor blue;
    XftColor magenta;
    XftColor cyan;
    XftColor white;
};

class XLibOutput final : public Output
{
public:
    XLibOutput();
    
    virtual int input_file() const override;
    virtual std::string update() override;
    
    ~XLibOutput();

private:
    virtual void redraw_all() override;
    virtual void draw_rune(const CursorPosition&, RuneMode mode = RuneMode::Normal) override;
    virtual void draw_scroll(int begin, int end, int by) override;
    virtual void flush_display() override;
    virtual void input(const std::string&) override;
    void draw_row(int row, bool refresh = false);
    void draw_update_selection(const CursorPosition &new_end_pos);
    void copy();
    void did_resize();
    std::string paste();

    template<typename CallbackFunc>
    void for_rune_in_selection(CallbackFunc callback);

    std::string decode_key_press(XKeyEvent *key_event);
    std::string decode_key_release(XKeyEvent *key_event);
    CursorPosition cursor_position_from_pixels(int x, int y);
    XftColor &text_color_from_terminal(TerminalColor color);
    void load_font(const std::string &&name, int size);
    void load_back_buffer(XVisualInfo&);
    void select_word_under_mouse();
    
    int m_width { 0 };
    int m_height { 0 };
    int m_depth { 0 };
    int m_font_width { 0 };
    int m_font_height { 0 };
    int m_scroll_offset { 0 };
    bool m_in_selection { false };
    CursorPosition m_selection_start;
    CursorPosition m_selection_end;
    CursorPosition m_mouse_pos;
    std::string m_input_buffer;
    
    // Xft
    XftFont *m_font;
    XftDraw *m_draw;
    XftPallet m_text_pallet;
    XIC m_input_context;
    
    // Xlib
    Display *m_display;
    Visual *m_visual;
    Window m_window;
    XdbeBackBuffer m_back_buffer;
    GC m_gc;
    int m_screen;
    int m_color_map;
    uint64_t m_time_after_last_click { 0 };
    Atom m_wm_delete_message;
    std::unique_ptr<XClipBoard> m_clip_board;
    
};
