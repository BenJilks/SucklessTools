#include "row.hpp"
#include "entry.hpp"
#include "column.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
using namespace DB;

Row::Row(const std::vector<Column> &columns)
{
    size_t entry_offset = 0;
    for (const auto &column : columns)
    {
        m_entities.push_back({ column, entry_offset, column.null() });
        entry_offset += column.data_type().size();
    }
}

Row::Row(std::vector<std::string> select_columns, Row &&other)
{
    for (auto &entity : other.m_entities)
    {
        // Only copy in if it's in the selected columns,
        // then remove it from the list
        auto column = entity.column;
        auto index = std::find(select_columns.begin(), select_columns.end(), column.name());
        if (index != select_columns.end())
        {
            (*this)[column.name()] = std::move(entity.entry);
            select_columns.erase(index);
        }
    }
}

std::unique_ptr<Entry> &Row::operator [](const std::string &name)
{
    for (auto &entity : m_entities)
    {
        if (entity.column.name() == name)
            return entity.entry;
    }
    
    // TODO: Error: This column doesn't exist
    assert (false);
}

std::ostream &operator<<(std::ostream &stream, const Row& row)
{
    stream << "Row: { ";
    for (const auto &it : row)
    {
        if (it.second != nullptr)
            stream << it.first << ": " << *it.second << ", ";
    }
    stream << "}";
    return stream;
}

void Row::read(Chunk &chunk, size_t row_offset)
{
    for (auto &entitiy : m_entities)
    {
        auto &entry = entitiy.entry;
        auto offset = row_offset + entitiy.offset_in_row;
        entry = entitiy.column.read(chunk, offset);
    }
}

void Row::write(Chunk &chunk, size_t row_offset)
{
    for (const auto &entitiy : m_entities)
    {
        auto &entry = entitiy.entry;
        auto offset = row_offset + entitiy.offset_in_row;
        if (entry)
            entry->write(chunk, offset);
    }    
}
