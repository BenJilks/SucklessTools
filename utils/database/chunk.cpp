#include "chunk.hpp"
#include "config.hpp"
#include "database.hpp"
using namespace DB;

Chunk::Chunk(DB::DataBase& db, size_t header_offset)
    : m_db(db)
    , m_header_offset(header_offset)
{
    db.read_string(header_offset, m_type, 2);
    m_owner_id = db.read_byte(header_offset + 2);
    m_index = db.read_byte(header_offset + 3);
    m_size_in_bytes = db.read_int(header_offset + 4);
    m_padding_in_bytes = db.read_int(header_offset + 8);
    m_data_offset = header_offset + Config::chunk_header_size;
}

size_t Chunk::header_size() const
{
    return Config::chunk_header_size;
}

bool Chunk::is_active() const
{
    assert (!m_has_been_dropped);
    return m_db.m_active_chunk.get() == this;
}

uint8_t Chunk::read_byte(size_t offset)
{
    assert (!m_has_been_dropped);
    return m_db.read_byte(m_data_offset + offset);
}

int Chunk::read_int(size_t offset)
{
    assert (!m_has_been_dropped);
    return m_db.read_int(m_data_offset + offset);
}

std::string Chunk::read_string(size_t offset, size_t len)
{
    assert (!m_has_been_dropped);
    std::vector<char> buffer(len);
    m_db.read_string(m_data_offset + offset, buffer.data(), len);
    return std::string(buffer.data(), buffer.size());
}

void Chunk::check_size(size_t size)
{
    if (size > m_size_in_bytes + m_padding_in_bytes)
    {
        m_db.check_is_active_chunk(this);
        m_size_in_bytes = size;
        m_db.write_int(m_header_offset + 4, m_size_in_bytes);
    }
}

void Chunk::write_byte(size_t offset, uint8_t byte)
{
    assert (!m_has_been_dropped);
    check_size(offset + 1);
    m_db.write_byte(m_data_offset + offset, byte);
}

void Chunk::write_int(size_t offset, int i)
{
    assert (!m_has_been_dropped);
    check_size(offset + 4);
    m_db.write_int(m_data_offset + offset, i);
}

void Chunk::write_string(size_t offset, const std::string &str)
{
    assert (!m_has_been_dropped);
    check_size(offset + str.size());
    m_db.write_string(m_data_offset + offset, str);
}

void Chunk::drop()
{
    m_db.write_string(m_header_offset, "RM");
    m_has_been_dropped = true;
}

void Chunk::shrink_to(size_t offset)
{
    m_padding_in_bytes = m_size_in_bytes - offset;
    m_size_in_bytes = offset;

    m_db.write_int(m_header_offset + 4, m_size_in_bytes);
    m_db.write_int(m_header_offset + 8, m_padding_in_bytes);

    for (size_t i = m_size_in_bytes; i < m_padding_in_bytes; i++)
        write_byte(i, 0xCD);
}

std::ostream &operator <<(std::ostream &stream, const Chunk &chunk)
{
    stream << "Chunk { type = " << chunk.type() <<
        ", data_offset = " << chunk.data_offset() <<
        ", size = " << chunk.size_in_bytes() <<
        ", owner_id = " << chunk.owner_id() <<
        ", index = " << chunk.index() << " }";
    return stream;
}

