#include "table.hpp"
#include "database.hpp"
#include <cassert>
using namespace DB;

Table::Table(DataBase& db, Constructor constructor)
{
    m_name = constructor.m_name;
    m_header = db.new_chunk("TH", 0xCD, 0xCD);
    for (const auto &it : constructor.m_columns)
    {
        m_columns.push_back(Column(it.first, it.second));
        m_row_size += it.second.size();
    }

    // Create table object
    write_header();
    m_data = db.new_chunk("RD", 0xCD, 0xCD);
    m_name = constructor.m_name;
}

Table::Table(std::shared_ptr<Chunk> header)
    : m_header(header)
    , m_id(header->owner_id())
{
    size_t offset = 0;

    // Name
    auto name_len = header->read_byte(offset);
    m_name = header->read_string(offset + 1, name_len);
    offset += 1 + name_len;

    // Column and row count
    auto column_count = header->read_byte(offset);
    m_row_count = header->read_int(offset + 1);
    offset += 1 + sizeof(int);

    for (size_t i = 0; i < column_count; i++)
    {
        // Column name
        auto column_name_len = header->read_byte(offset);
        auto column_name = header->read_string(offset + 1, column_name_len);
        offset += 1 + column_name_len;

        // Column type
        auto primitive = static_cast<DataType::Primitive>(header->read_byte(offset) - 1);
        auto length = header->read_byte(offset + 1);
        auto type = DataType(primitive, 4, length); // TODO: Use real size, assuming 4, for now
        offset += 1 + 1;

        m_columns.push_back(Column(column_name, type));
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

        m_header->write_byte(curr_offset, name.size());
        m_header->write_string(curr_offset + 1, name);
        curr_offset += 1 + name.size();

        m_header->write_byte(curr_offset, type.primitive());
        m_header->write_byte(curr_offset, type.length());
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
    assert (m_data);

    Row::Constructor row(*m_data, m_row_count * m_row_size);
    m_row_count += 1;
    m_header->write_int(m_row_count_offset, m_row_count);
    return row;
}

std::optional<Row> Table::get_row(size_t index)
{
    assert (m_data);

    Row row(*m_data);
    auto row_offset = index * m_row_size;
    auto entry_offset = row_offset;
    for (const auto &column : m_columns)
    {
        row.m_entries[column.name()] = column.read(*m_data, entry_offset);
        entry_offset += column.data_type().size();
    }

    return std::move(row);
}

void Table::add_row_data(std::shared_ptr<Chunk> data)
{
    // NOTE: We only have one chunk, for now
    assert (!m_data);
    m_data = std::move(data);
}
