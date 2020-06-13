#include "column.hpp"
using namespace DB;

std::unique_ptr<Entry> Column::read(Chunk &chunk, size_t offset) const
{
    switch (m_data_type.primitive())
    {
        case DataType::Integer: return std::make_unique<IntegerEntry>(chunk, offset);
        default:
            // TODO: Error
            assert (false);
            return nullptr;
    }
}
