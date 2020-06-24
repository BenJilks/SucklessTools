#include "config.hpp"
#include "row.hpp"
#include "entry.hpp"
#include "column.hpp"
#include "chunk.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
using namespace DB;

Row::Row(const std::vector<Column> &columns)
{
    size_t entry_offset = Config::row_header_size;
    for (const auto &column : columns)
    {
        m_entities.push_back({ column, entry_offset, column.null() });
        entry_offset += column.data_type().size();
    }

    m_row_size = entry_offset;
}

Row::Row(std::vector<std::string> select_columns, Row &&other)
{
    size_t entry_offset = Config::row_header_size;
    for (auto &entity : other.m_entities)
    {
        // Only copy in if it's in the selected columns,
        // then remove it from the list
        auto column = entity.column;
        auto index = std::find(select_columns.begin(), select_columns.end(), column.name());
        if (index != select_columns.end())
        {
            m_entities.push_back({ column, entry_offset, std::move(entity.entry)});
            select_columns.erase(index);
        }
        
        entry_offset += column.data_type().size();
    }
    m_row_size = other.m_row_size;
}

std::unique_ptr<Entry> const &Row::operator [](const std::string &name)
{
    for (auto &entity : m_entities)
    {
        if (entity.column.name() == name)
            return entity.entry;
    }

    // TODO: Error: This column doesn't exist
    assert (false);
}

const std::unique_ptr<Entry> &Row::operator [](const std::string &name) const
{
    for (const auto &entity : m_entities)
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
    for (size_t i = row_offset; i < m_row_size + 1; i++)
        chunk.write_byte(i, 0xCD);

    for (const auto &entitiy : m_entities)
    {
        auto &entry = entitiy.entry;
        auto offset = row_offset + entitiy.offset_in_row;
        if (entry)
            entry->write(chunk, offset);
    }
}
