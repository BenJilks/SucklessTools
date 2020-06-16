#include "table.hpp"
#include "database.hpp"
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

    for (size_t i = 0; i < column_count; i++)
    {
        // Column name
        auto column_name_len = (size_t)header->read_byte(offset);
        auto column_name = header->read_string(offset + 1, column_name_len);
        offset += 1 + column_name_len;

        // Column type
        auto primitive = static_cast<DataType::Primitive>(header->read_byte(offset));
        auto length = header->read_byte(offset + 1);
        auto type = DataType(primitive, 4, length); // TODO: Use real size, assuming 4, for now
        offset += 1 + 1;

#ifdef DEBUG_TABLE_LOAD
        std::cout << "Loaded Column { " <<
            "name_length = " << column_name_len << ", " <<
            "name = " << column_name << ", " <<
            "primitive = " << primitive << " }\n";
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

std::optional<Row> Table::add_row(Row::Constructor constructor)
{
    const auto &data = constructor.m_entries;
    if (data.size() != m_columns.size())
    {
        // TODO: Error
        assert (false);
        return std::nullopt;
    }

    for (size_t i = 0; i < data.size(); i++)
    {
        auto &entry = data[i];
        if (entry->type() != m_columns[i].m_data_type)
        {
            // TODO: Error
            assert (false);
            return std::nullopt;
        }
        entry->write();
    }

    return Row(std::move(constructor), m_columns);
}

Row::Constructor Table::new_row()
{
    std::shared_ptr<Chunk> active_chunk = nullptr;
    if (m_row_data_chunks.size() > 0)
        active_chunk = m_row_data_chunks.back();

    if (!active_chunk || !active_chunk->is_active())
    {
        active_chunk = m_db.new_chunk("RD", m_id, find_next_row_chunk_index());
        m_row_data_chunks.push_back(active_chunk);
    }

    auto row_count_in_chunk = active_chunk->size_in_bytes() / m_row_size;
    Row::Constructor row(*active_chunk, row_count_in_chunk * m_row_size);
    m_row_count += 1;
    m_header->write_int(m_row_count_offset, m_row_count);
    return row;
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
    std::shared_ptr<Chunk> chunk_containing_row = nullptr;
    size_t curr_max_row_count = 0;
    size_t row_count_at_start_of_chunk = 0;

    for (const auto &chunk : m_row_data_chunks)
    {
        auto chunk_size_in_bytes = chunk->size_in_bytes();
        auto chunk_row_count = chunk_size_in_bytes / m_row_size;
        row_count_at_start_of_chunk = curr_max_row_count;
        curr_max_row_count += chunk_row_count;

        if (index < curr_max_row_count)
        {
            chunk_containing_row = chunk;
            break;
        }
    }

    // No chunk found
    if (chunk_containing_row == nullptr)
        return std::nullopt;

    Row row(*chunk_containing_row);
    auto row_offset = (index - row_count_at_start_of_chunk) * m_row_size;
    auto entry_offset = row_offset;
    for (const auto &column : m_columns)
    {
        row.m_entries[column.name()] = column.read(*chunk_containing_row, entry_offset);
        entry_offset += column.data_type().size();
    }

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

void Table::drop()
{
    m_header->drop();
    for (const auto &chunk : m_row_data_chunks)
        chunk->drop();
}
