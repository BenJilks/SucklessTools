#pragma once
#include "../line.hpp"

class Row
{
public:
    Row() = default;

    inline bool is_dirty() const { return m_is_dirty; }
    inline void set_dirty(bool f) { m_is_dirty = f; }
    inline bool was_in_selection() const { return m_was_in_selection; }
    inline void set_was_in_selection(bool f) { m_was_in_selection = f; }

    inline void set_line(Line *line) { m_line = line; m_is_dirty = true; }
    inline bool has_line() const { return m_line; }
    Line &line()
    {
        assert (m_line);
        return *m_line;
    }

private:
    Line *m_line { nullptr };
    bool m_is_dirty { true };
    bool m_was_in_selection { true };

};
