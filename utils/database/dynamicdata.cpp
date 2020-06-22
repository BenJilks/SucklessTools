#include "database.hpp"
#include "dynamicdata.hpp"
#include "chunk.hpp"
#include <cassert>
using namespace DB;

void DynamicData::set(const std::vector<char> &data)
{
    assert (m_chunk);

    auto &db = m_chunk->m_db;

    // Remove padding and make it part of the total size
    m_chunk->m_size_in_bytes += m_chunk->m_padding_in_bytes;
    m_chunk->m_padding_in_bytes = 0;

    // If this new data does not fit the current chunk, make a new one
    if (!m_chunk->is_active() && data.size() > m_chunk->size_in_bytes())
    {
        auto new_chunk = db.new_chunk("DY", m_chunk->owner_id(), m_chunk->index());
        m_chunk->drop();
        m_chunk = new_chunk;
    }

    // Copy data into chunk
    for (size_t i = 0; i < data.size(); i++)
        m_chunk->write_byte(i, data[i]);

    // Shrink chunk to fit
    m_chunk->shrink_to(data.size());
}

std::vector<char> DynamicData::read()
{
    assert (m_chunk);

    std::vector<char> buffer(m_chunk->size_in_bytes());
    for (size_t i = 0; i < buffer.size(); i++)
        buffer[i] = m_chunk->read_byte(i);

    return buffer;
}

int DynamicData::id() const
{
    return m_chunk->index();
}
