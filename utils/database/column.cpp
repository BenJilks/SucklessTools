#include "column.hpp"
#include <cassert>
#include <iostream>
using namespace DB;

std::unique_ptr<Entry> Column::read(Chunk &chunk, size_t offset) const
{
    std::unique_ptr<Entry> entry = null();
    entry->read(chunk, offset);
    return entry;
}

std::unique_ptr<Entry> Column::null() const
{
    switch (m_data_type.primitive())
    {
        case DataType::Integer: return std::make_unique<IntegerEntry>();
        case DataType::Char: return std::make_unique<CharEntry>(m_data_type.length());
        case DataType::Text: return std::make_unique<TextEntry>();
        default:
            // TODO: Error
            assert (false);
            return nullptr;
    }
}
