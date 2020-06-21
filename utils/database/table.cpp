#include "config.hpp"
#include "chunk.hpp"
#include "table.hpp"
#include "database.hpp"
#include "dynamicdata.hpp"
#include <algorithm>
#include <cassert>
using namespace DB;

// #define DEBUG_TABLE_LOAD

Table::Table(DataBase& db, Constructor constructor)
    : m_db(db)
{
    m_id = db.generate_table_id();
    m_name = constructor.m_name;
    m_header = db.new_chunk("TH", m_id, 0xCD);
    m_row_size = Config::row_header_size;
    for (const auto &it : constructor.m_columns)
    {
        m_columns.push_back(Column(it.first, it.second));
        m_row_size += it.second.size();
    }

    // Create table object
    write_header();
    m_name = constructor.m_name;
}

Table::Table(DataBase &db, std::shared_ptr<Chunk> header)
    : m_db(db)
    , m_header(header)
    , m_id(header->owner_id())
{
    size_t offset = 0;

    // Name
    auto name_len = header->read_byte(offset);
    m_name = header->read_string(offset + 1, name_len);
    offset += 1 + name_len;

    // Column and row count
    auto column_count = header->read_byte(offset);
    m_row_count_offset = offset + 1;
    m_row_count = header->read_int(offset + 1);
    offset += 1 + sizeof(int);

    m_row_size = Config::row_header_size;
    for (size_t i = 0; i < column_count; i++)
    {
        // Column name
        auto column_name_len = (size_t)header->read_byte(offset);
        auto column_name = header->read_string(offset + 1, column_name_len);
        offset += 1 + column_name_len;

        // Column type
        auto primitive = static_cast<DataType::Primitive>(header->read_byte(offset));
        auto length = header->read_byte(offset + 1);
        auto type = DataType(primitive, DataType::size_from_primitive(primitive), length);
        offset += 1 + 1;

#ifdef DEBUG_TABLE_LOAD
        std::cout << "Loaded Column { " <<
            "name_length = " << column_name_len << ", " <<
            "name = " << column_name << ", " <<
            "primitive = " << primitive << ", " <<
            "offset = " << m_row_size << " }\n";
#endif
        m_columns.push_back(Column(column_name, type));
        m_row_size += type.size();
    }

    std::cout << "Loaded Table { " <<
        "name = " << m_name <<
        ", column_count = " << (int)column_count <<
        ", row_count = " << m_row_count << " }\n";
}

void Table::write_header()
{
    size_t curr_offset = 0;

    m_header->write_byte(curr_offset, m_name.size());
    m_header->write_string(curr_offset + 1, m_name);
    curr_offset += 1 + m_name.size();

    m_header->write_byte(curr_offset, m_columns.size());
    m_row_count_offset = curr_offset + 1;
    m_header->write_int(curr_offset + 1, 0);
    curr_offset += 1 + sizeof(int);

    for (const auto &column : m_columns)
    {
        const auto &name = column.m_name;
        const auto &type = column.m_data_type;

#ifdef DEBUG_TABLE_LOAD
        std::cout << "Write column " << name << " of size " << (int)(uint8_t)name.size() << "\n";
#endif
        m_header->write_byte(curr_offset, (uint8_t)name.size());
        m_header->write_string(curr_offset + 1, name);
        curr_offset += 1 + name.size();

        m_header->write_byte(curr_offset, (int)type.primitive());
        m_header->write_byte(curr_offset + 1, type.length());
        curr_offset += 2;
    }

    // TODO: Add this API
    // m_header.flush();
}

std::unique_ptr<DynamicData> Table::new_dynamic_data()
{
    size_t max_id = 0;
    for (const auto &chunk : m_dynamic_data_chunks)
        max_id = std::max(max_id, chunk->index());

    auto chunk = m_db.new_chunk("DY", m_id, max_id + 1);
    m_dynamic_data_chunks.push_back(chunk);
    return std::make_unique<DynamicData>(chunk);
}

void Table::add_row(Row row)
{
    // TODO: There's much better ways of checking if
    //       this row belongs to a table
    if (row.m_entities.size() != m_columns.size())
    {
        // TODO: Error
        assert (false);
        return;
    }

    // Find or create the active chunk
    std::shared_ptr<Chunk> active_chunk;
    auto new_chunk = [&]() {
        auto chunk = m_db.new_chunk("RD", m_id, find_next_row_chunk_index());
        m_row_data_chunks.push_back(chunk);
        return chunk;
    };

    if (m_row_data_chunks.size() <= 0)
    {
        active_chunk = new_chunk();
    }
    else
    {
        active_chunk = m_row_data_chunks.back();
        if (!active_chunk->is_active())
            active_chunk = new_chunk();
    }

    // Write the row to disk
    auto offset = active_chunk->size_in_bytes();
    row.write(*active_chunk, offset);

    // Update row count
    m_row_count += 1;
    m_header->write_int(m_row_count_offset, m_row_count);
}

void Table::update_row(size_t index, Row row)
{
    auto [chunk, offset] = find_chunk_and_offset_for_row(index);
    assert (chunk);

    row.write(*chunk, offset);
}

void Table::remove_row(size_t index)
{
    auto [chunk, offset] = find_chunk_and_offset_for_row(index);

    // Insert a new chunk inbetween the one this row is in and the rest
    auto new_chunk = m_db.new_chunk("RD", m_id, chunk->index() + 1);
    for (auto &it : m_row_data_chunks)
    {
        if (it->index() > chunk->index())
            it->increment_index(1);
    }

    // Copy data to knew chunk
    for (size_t i = offset + m_row_size; i < chunk->size_in_bytes(); i++)
        new_chunk->write_byte(i - (offset + m_row_size), chunk->read_byte(i));

    // Shrink chunk to before the deleted row
    chunk->shrink_to(offset);

    // Re-sort chunks
    m_row_data_chunks.push_back(new_chunk);
    std::sort(m_row_data_chunks.begin(), m_row_data_chunks.end(), [](const auto &a, const auto &b)
    {
        return a->index() < b->index();
    });

    // Update row count
    m_row_count -= 1;
    m_header->write_int(m_row_count_offset, m_row_count);
}

Row Table::make_row()
{
    return Row(m_columns);
}

std::tuple<std::shared_ptr<Chunk>, size_t> Table::find_chunk_and_offset_for_row(size_t row)
{
    std::shared_ptr<Chunk> chunk_containing_row = nullptr;
    size_t curr_max_row_count = 0;
    size_t row_count_at_start_of_chunk = 0;

    for (const auto &chunk : m_row_data_chunks)
    {
        auto chunk_size_in_bytes = chunk->size_in_bytes();
        auto chunk_row_count = chunk_size_in_bytes / m_row_size;
        row_count_at_start_of_chunk = curr_max_row_count;
        curr_max_row_count += chunk_row_count;

        if (row < curr_max_row_count)
        {
            chunk_containing_row = chunk;
            break;
        }
    }

    // No chunk found
    if (chunk_containing_row == nullptr)
        return std::make_tuple(nullptr, 0);

    auto row_offset = (row - row_count_at_start_of_chunk) * m_row_size;
    return std::make_tuple(chunk_containing_row, row_offset);
}

int Table::find_next_row_chunk_index()
{
    int max_index = 0;
    for (const auto &chunk : m_row_data_chunks)
        max_index = std::max(max_index, (int)chunk->index());

    return max_index + 1;
}

std::optional<Row> Table::get_row(size_t index)
{
    auto [chunk, offset] = find_chunk_and_offset_for_row(index);
    assert (chunk);

    Row row(m_columns);
    row.read(*chunk, offset);
    return std::move(row);
}

void Table::add_row_data(std::shared_ptr<Chunk> data)
{
    m_row_data_chunks.push_back(std::move(data));

    // Make sure chunks are in the correct order (sort by index)
    std::sort(m_row_data_chunks.begin(), m_row_data_chunks.end(), [](const auto &a, const auto &b)
    {
        return a->index() < b->index();
    });
}

void Table::add_dynamic_data(std::shared_ptr<Chunk> data)
{
    m_dynamic_data_chunks.push_back(std::move(data));
}

std::shared_ptr<Chunk> Table::find_dynamic_chunk(int id)
{
    for (auto &chunk : m_dynamic_data_chunks)
    {
        if (chunk->index() == id)
            return chunk;
    }

    return nullptr;
}

void Table::drop()
{
    m_header->drop();
    for (const auto &chunk : m_row_data_chunks)
        chunk->drop();
}
