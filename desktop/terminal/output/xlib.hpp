#include "output.hpp"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <vector>

class XLibOutput final : public Output
{
public:
    XLibOutput();
    
    virtual void write(std::string_view buff) override;
    virtual int input_file() const override;
    virtual std::string update() override;
    
    ~XLibOutput();

private:
    std::string decode_key_press(XKeyEvent *key_event);
    void load_font(const std::string &&name, int size);
    void draw_window() const;
    void draw_buffer() const;
    std::vector<std::string> lines() const;

    std::string m_buffer;
    int m_cursor;
    
    // Xft
    XftFont *m_font;
    XftDraw *m_draw;
    XftColor m_color;
    XIC m_input_context;
    
    // Xlib
    Display *m_display;
    Visual *m_visual;
    Window m_window;
    int m_screen;
    int m_color_map;
    
};
