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

void Entry::read(Chunk &chunk, size_t offset)
{
    m_is_null = chunk.read_byte(offset);
    read_data(chunk, offset + 1);    
}

void Entry::write(Chunk &chunk, size_t offset)
{
    chunk.write_byte(offset, m_is_null);
    write_data(chunk, offset + 1);
}

void IntegerEntry::write_data(Chunk &chunk, size_t offset)
{
    chunk.write_int(offset, m_i);
}

void IntegerEntry::read_data(Chunk &chunk, size_t offset)
{
    m_i = chunk.read_int(offset);
}

std::ostream &operator<< (std::ostream &stream, const DB::Entry& entry)
{
    if (entry.is_null())
    {
        stream << "NULL";
        return stream;
    }
    
    switch (entry.type().primitive())
    {
        case DataType::Integer:
            stream << static_cast<const IntegerEntry&>(entry).data();
            break;
    }

    return stream;
}
