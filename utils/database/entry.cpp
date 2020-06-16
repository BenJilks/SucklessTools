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

void IntegerEntry::write(Chunk &chunk, size_t offset)
{
    chunk.write_int(offset, m_i);
}

void IntegerEntry::read(Chunk &chunk, size_t offset)
{
    m_i = chunk.read_int(offset);
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
