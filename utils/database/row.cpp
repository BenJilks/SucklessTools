#include "row.hpp"
#include "entry.hpp"
#include "column.hpp"
#include <algorithm>
#include <iostream>
using namespace DB;

Row::Row(Constructor&& constructor, const std::vector<Column> &columns)
{
    for (size_t i = 0; i < constructor.m_entries.size(); i++)
    {
        auto &entry = constructor.m_entries[i];
        m_entries[columns[i].name()] = std::move(entry);
    }
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

Entry &Row::operator [](const std::string &name)
{
    return *m_entries[name];
}

void Row::Constructor::integer_entry(int i)
{
    auto entry = std::make_unique<IntegerEntry>(i);
    m_entries.push_back(std::move(entry));
}

std::ostream &operator<<(std::ostream &stream, const Row& row)
{
    stream << "Row: { ";
    for (const auto &it : row)
        stream << it.first << ": " << *it.second << ", ";
    stream << "}";
    return stream;
}

