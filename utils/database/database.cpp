#include "config.hpp"
#include "chunk.hpp"
#include "database.hpp"
#include "sql/parser.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <filesystem>
using namespace DB;

std::shared_ptr<Chunk> DataBase::new_chunk(std::string_view type, uint8_t owner_id, uint8_t index)
{
    auto chunk = std::shared_ptr<Chunk>(new Chunk(*this));
    memcpy(chunk->m_type, type.data(), 2);
    chunk->m_owner_id = owner_id;
    chunk->m_index = index;
    chunk->m_header_offset = m_end_of_data_pointer;
    write_byte(chunk->m_header_offset + 0, type[0]);
    write_byte(chunk->m_header_offset + 1, type[1]);
    write_byte(chunk->m_header_offset + 2, owner_id);
    write_byte(chunk->m_header_offset + 3, index);
    write_int(chunk->m_header_offset + 4, 0);
    write_int(chunk->m_header_offset + 8, 0);
    write_int(chunk->m_header_offset + 12, 0);
    write_int(chunk->m_header_offset + 16, 0);

    chunk->m_data_offset = chunk->m_header_offset + Config::chunk_header_size;
#ifdef DEBUG_CHUNKS
    std::cout << "New chunk { type = " << type <<
        ", header_offset = " << chunk->m_header_offset <<
        ", data_offset = " << chunk->m_data_offset <<
        ", owner = " << (int)chunk->m_owner_id << " }\n";
#endif

    m_chunks.push_back(chunk);
    m_active_chunk = m_chunks.back();
    return m_chunks.back();
}

void DataBase::check_is_active_chunk(Chunk *chunk)
{
    // NOTE: We have to be the active chunk to append data
    assert (m_active_chunk.get() == chunk);
}

std::shared_ptr<DataBase> DataBase::open(const std::string& path)
{
    FILE *file;

    if (!std::filesystem::exists(path))
        file = fopen(path.c_str(), "w+b");
    else
        file = fopen(path.c_str(), "r+b");

    if (!file)
    {
        perror("fopen()");
        return nullptr;
    }

    return std::shared_ptr<DataBase>(new DataBase(file));
}

DataBase::DataBase(FILE *file)
    : m_file(file)
{
    // Find file length
    fseek(m_file, 0L, SEEK_END);
    m_end_of_data_pointer = ftell(m_file);
    rewind(m_file);

    // Load existing chunks
    size_t offset = 0;
    while (offset < m_end_of_data_pointer)
    {
        auto chunk = std::shared_ptr<Chunk>(new Chunk(*this, offset));
        offset += chunk->header_size() +
            chunk->size_in_bytes() +
            chunk->padding_in_bytes();

        if (chunk->type() == "RM")
        {
#ifdef DEBUG_CHUNKS
            std::cout << "Dropped chunk " <<
                "at: " << chunk->data_offset() <<
                " of size: " << chunk->size_in_bytes() << "\n";
#endif
            continue;
        }

#ifdef DEBUG_CHUNKS
        std::cout << "Loaded " << *chunk << "\n";
#endif
        if (chunk->type() == "TH")
        {
            // TableHeader
            m_tables.push_back(Table(*this, chunk));
        }
        else if (chunk->type() == "RD")
        {
            // RowData
            auto *table = find_owner(chunk->owner_id());
            assert (table);

            table->add_row_data(chunk);
        }
        else if (chunk->type() == "VR")
        {
            // Version
            m_version_chunk = chunk;
        }
        else if (chunk->type() == "DY")
        {
            // Dynamic Data
            auto *table = find_owner(chunk->owner_id());
            assert (table);

            table->add_dynamic_data(chunk);
        }

        m_chunks.push_back(chunk);
        m_active_chunk = chunk;
    }

    if (!m_version_chunk)
        write_version_chunk();
}

void DataBase::write_version_chunk()
{
    m_version_chunk = new_chunk("VR", 0, 0);
    m_version_chunk->write_byte(0, Config::major_version);
    m_version_chunk->write_byte(1, Config::minor_version);
}

SqlResult DataBase::execute_sql(const std::string &query)
{
    std::cout << "DataBase: Executing SQL '" << query << "'\n";

    Sql::Parser parser(query);
    auto statement = parser.run();
    if (!parser.good())
        return parser.errors_as_result();

    return statement->execute(*this);
}

uint8_t DataBase::generate_table_id()
{
    uint8_t max_id = 0;
    for (const auto &table : m_tables)
        max_id = std::max(max_id, (uint8_t)table.id());

    return max_id + 1;
}

void DataBase::check_size(size_t size)
{
    if (size > m_end_of_data_pointer)
        m_end_of_data_pointer = size;
}

void DataBase::write_byte(size_t offset, char byte)
{
    check_size(offset + 1);
    fseek(m_file, offset, SEEK_SET);
    fwrite(&byte, 1, 1, m_file);
    fflush(m_file);
}

void DataBase::write_int(size_t offset, int i)
{
    check_size(offset + 4);
    fseek(m_file, offset, SEEK_SET);
    fwrite((char*)(&i), 1, 4, m_file);
    fflush(m_file);
}

void DataBase::write_string(size_t offset, const std::string& str)
{
    check_size(offset + str.size());
    fseek(m_file, offset, SEEK_SET);
    fwrite(str.data(), 1, str.size(), m_file);
    fflush(m_file);
}

void DataBase::flush()
{
    fflush(m_file);
}

uint8_t DataBase::read_byte(size_t offset)
{
    uint8_t byte;
    fseek(m_file, offset, SEEK_SET);
    fread(&byte, 1, 1, m_file);
    return byte;
}

int DataBase::read_int(size_t offset)
{
    int i;
    fseek(m_file, offset, SEEK_SET);
    fread(&i, 1, sizeof(int), m_file);
    return i;
}

void DataBase::read_string(size_t offset, char *str, size_t len)
{
    fseek(m_file, offset, SEEK_SET);
    fread(str, 1, len, m_file);
}

Table &DataBase::construct_table(Table::Constructor constructor)
{
    m_tables.push_back(Table(*this, constructor));
    return m_tables.back();
}

Table *DataBase::find_owner(uint8_t owner_id)
{
    for (auto &table : m_tables)
    {
        if (table.id() == owner_id)
            return &table;
    }

    return nullptr;
}

Table *DataBase::get_table(const std::string &name)
{
    for (auto &table : m_tables)
    {
        if (table.name() == name)
            return &table;
    }

    return nullptr;
}

bool DataBase::drop_table(const std::string &name)
{
    auto table_index = m_tables.end();
    for (auto it = m_tables.begin(); it != m_tables.end(); ++it)
    {
        if (it->name() == name)
        {
            table_index = it;
            break;
        }
    }

    if (table_index == m_tables.end())
        return false;

    table_index->drop();
    m_tables.erase(table_index);
    return true;
}

DataBase::~DataBase()
{
    if (m_file)
        fclose(m_file);
}
