#pragma once
#include "output.hpp"
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <string>
#include <vector>
#include <iostream>

class Line
{
public:
    Line() {}

    inline void set(int coloumn, char c)
    {
        while (coloumn >= m_data.length())
            m_data += ' ';
        m_data[coloumn] = c;
        
        m_is_dirty = true;
    }
    
    inline void insert(int coloumn, char c)
    {
        if (coloumn >= m_data.length())
            set(coloumn, c);
        
        m_data.insert(coloumn, std::string(&c, 1));
        m_is_dirty = true;
    }
    
    inline void erase(int coloumn, int count)
    {
        m_data.erase(coloumn, count);
        m_is_dirty = true;
    }
    
    inline bool is_dirty() const { return m_is_dirty; }
    inline void mark_dirty() { m_is_dirty = true; }
    inline void unmark_dirty() { m_is_dirty = false; }
    inline const std::string &data() const { return m_data; }

private:
    std::string m_data;
    bool m_is_dirty { false };
    
};

class CursorPosition
{
public:
    CursorPosition(int coloumn, int row)
        : m_coloumn(coloumn), m_row(row) {}
    
    CursorPosition()
        : m_coloumn(0), m_row(0) {}
    
    inline int& coloumn() { return m_coloumn; }
    inline int& row() { return m_row; }
    inline int coloumn() const { return m_coloumn; }
    inline int row() const { return m_row; }

    inline void move_to(int coloumn, int row) 
    { 
        m_coloumn = std::max(coloumn, 0); 
        m_row = std::max(row, 0);
    }
    inline void move_to_begging_of_line() { m_coloumn = 0; }

    inline void move_by(int coloumn, int row) 
    {
        m_coloumn = std::max(m_coloumn + coloumn, 0);
        m_row = std::max(m_row + row, 0);
    }
    
private:
    int m_coloumn { 0 };
    int m_row { 0 };
    
};

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
    void redraw_all();
    void draw_window();
    void draw_buffer();
    Line &line_at(const CursorPosition &position);

    std::vector<Line> m_lines;
    CursorPosition m_cursor;
    int m_curr_frame_index { 0 };
    int m_width { 0 };
    int m_height { 0 };
    int m_rows { 1024 };
    
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
