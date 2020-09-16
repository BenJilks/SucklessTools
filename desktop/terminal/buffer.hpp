#pragma once
#include "attribute.hpp"
#include "cursor.hpp"
#include <stdint.h>

class Buffer
{
public:
    struct Rune
    {
        uint32_t value;
        Attribute attribute;
    };

    static Rune blank_rune();

    Buffer() = default;
    Buffer(int rows, int columns);

    void resize(int rows, int columns);
    void clear_row(int row);
    void scroll(int top, int botton, int by);
    void set_mode(int mode, bool value);
    Rune &rune_at(const CursorPosition&);
    const Rune &rune_at_scroll_offset(const CursorPosition&, int offset) const;
    const Rune &rune_at(const CursorPosition&) const;

    inline int rows() const { return m_rows; }
    inline int columns() const { return m_columns; }
    inline int scroll_back() const { return m_scroll_back_rows; }

    inline CursorPosition &cursor() { return m_cursor; }
    inline Attribute &current_attribute() { return m_current_attribute; }
    inline const CursorPosition &cursor() const { return m_cursor; }
    inline const Attribute &current_attribute() const { return m_current_attribute; }

    inline int scroll_region_top() const { return m_scroll_region_top; }
    inline int scroll_region_bottom() const { return m_scroll_region_bottom; }
    inline bool auto_wrap_mode() const { return m_auto_wrap_mode; }
    inline bool relative_origin_mode() const { return m_relative_origin_mode; }
    inline bool application_keys_mode() const { return m_application_keys_mode; }

    void set_scroll_region(int top, int bottom);

private:
    Rune *m_buffer { nullptr };
    Rune *m_scroll_back { nullptr };
    Rune m_invalid_rune { ' ', {} };
    int m_rows { 80 };
    int m_columns { 80 };
    int m_scroll_back_rows { 0 };

    CursorPosition m_cursor;
    Attribute m_current_attribute;

    int m_scroll_region_top { 0 };
    int m_scroll_region_bottom { 79 };
    bool m_auto_wrap_mode { true };
    bool m_relative_origin_mode { false };
    bool m_application_keys_mode { false };

    Rune *allocate_new_buffer(Rune *buffer,
        int new_rows, int new_columns,
        int old_rows, int old_columns);
};
