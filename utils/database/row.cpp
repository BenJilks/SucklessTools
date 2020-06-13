#include "row.hpp"
#include "entry.hpp"
#include "column.hpp"
#include <iostream>
using namespace DB;

Row::Constructor::Constructor(Chunk &chunk, size_t row_offset)
    : m_chunk(chunk)
    , m_row_offset(row_offset) {}

Row::Row(Constructor&& constructor, const std::vector<Column> &columns)
    : m_chunk(constructor.m_chunk)
{
    for (size_t i = 0; i < constructor.m_entries.size(); i++)
    {
        auto &entry = constructor.m_entries[i];
        m_entries[columns[i].name()] = std::move(entry);
    }
}

Entry &Row::operator [](const std::string &name)
{
    return *m_entries[name];
}

void Row::Constructor::integer_entry(int i)
{
    auto offset = m_row_offset + m_curr_entry_offset;
    auto entry = std::make_unique<IntegerEntry>(m_chunk, offset, i);

    m_curr_entry_offset += entry->type().size();
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

