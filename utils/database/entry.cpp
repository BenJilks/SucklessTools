#include "entry.hpp"
#include "database.hpp"
using namespace DB;

bool DataType::operator== (const DataType &other) const
{
    return m_primitive == other.m_primitive &&
        m_length == other.m_length;
}

DataType DataType::integer()
{
    return DataType(Integer, 4, 1);
}

IntegerEntry::IntegerEntry(Chunk &chunk, size_t data_offset)
    : Entry(chunk, DataType::integer(), data_offset)
{
    m_i = chunk.read_int(data_offset);
}

void IntegerEntry::write()
{
    chunk().write_int(data_offset(), m_i);
}

std::ostream &operator<< (std::ostream &stream, const DB::Entry& entry)
{
    switch (entry.type().primitive())
    {
        case DataType::Integer:
            stream << static_cast<const IntegerEntry&>(entry).data();
            break;
    }

    return stream;
}
