#include "row.hpp"
#include "entry.hpp"
#include "column.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
using namespace DB;

Row::Row(const std::vector<Column> &columns)
{
    for (const auto &column : columns)
        m_entries[column.name()] = nullptr;
}

Row::Row(std::vector<std::string> select_columns, Row &&other)
{
    for (auto &column : other.m_entries)
    {
        // Only copy in if it's in the selected columns,
        // then remove it from the list
        auto index = std::find(select_columns.begin(), select_columns.end(), column.first);
        if (index != select_columns.end())
        {
            m_entries[column.first] = std::move(column.second);
            select_columns.erase(index);
        }
    }
}

std::unique_ptr<Entry> &Row::operator [](const std::string &name)
{
    if (m_entries.find(name) == m_entries.end())
    {
        // TODO: Error: This column doesn't exist
        assert (false);
    }
    
    return m_entries[name];
}

std::ostream &operator<<(std::ostream &stream, const Row& row)
{
    stream << "Row: { ";
    for (const auto &it : row)
        stream << it.first << ": " << *it.second << ", ";
    stream << "}";
    return stream;
}

