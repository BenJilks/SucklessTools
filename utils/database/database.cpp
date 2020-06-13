#include "database.hpp"
#include <cassert>
#include <fstream>
#include <filesystem>
using namespace DB;

Chunk::Chunk(DB::DataBase& db, size_t header_offset)
    : m_db(db)
    , m_header_offset(header_offset)
{
    db.read_string(header_offset, m_type, 2);
    m_owner_id = db.read_byte(header_offset + 2);
    m_index = db.read_byte(header_offset + 3);
    m_size_in_bytes = db.read_int(header_offset + 4);
    m_data_offset = header_offset + 8;
}

uint8_t Chunk::read_byte(size_t offset)
{
    return m_db.read_byte(m_data_offset + offset);
}

int Chunk::read_int(size_t offset)
{
    return m_db.read_int(m_data_offset + offset);
}

std::string Chunk::read_string(size_t offset, size_t len)
{
    std::vector<char> buffer(len);
    m_db.read_string(m_data_offset + offset, buffer.data(), len);
    return std::string(buffer.data(), buffer.size());
}

void Chunk::check_size(size_t size)
{
    if (size >= m_size_in_bytes)
    {
        m_db.check_is_active_chunk(this);
        m_size_in_bytes = size;
        m_db.write_int(m_header_offset + 4, m_size_in_bytes);
    }
}

void Chunk::write_byte(size_t offset, uint8_t byte)
{
    check_size(offset + 1);
    m_db.write_byte(m_data_offset + offset, byte);
}

void Chunk::write_int(size_t offset, int i)
{
    check_size(offset + 4);
    m_db.write_int(m_data_offset + offset, i);
}

void Chunk::write_string(size_t offset, const std::string &str)
{
    check_size(offset + str.size());
    m_db.write_string(m_data_offset + offset, str);
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

    chunk->m_data_offset = m_end_of_data_pointer;
    std::cout << "New chunk { type = " << type <<
        ", header_offset = " << chunk->m_header_offset <<
        ", data_offset = " << chunk->m_data_offset << " }\n";

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

    /*if (!std::filesystem::exists(path))
        file = fopen(path.c_str(), "w+");
    else
        file = fopen(path.c_str(), "r+");*/
    file = fopen(path.c_str(), "r+");

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
        offset += 8 + chunk->size_in_bytes();

        std::cout << "Loaded " << *chunk << "\n";
        if (chunk->type() == "TH")
        {
            // TableHeader
            assert (!m_table);
            m_table = new Table(chunk);
        }
        else if (chunk->type() == "RD")
        {
            // RowData
            assert(m_table); // NOTE: As we only have one table, assert that it exists
            assert(m_table->id() == chunk->owner_id()); // Make sure ID's match
            m_table->add_row_data(chunk);
            std::cout << "Attached row data to chunk\n";
        }

        m_chunks.push_back(chunk);
    }
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
}

void DataBase::write_int(size_t offset, int i)
{
    check_size(offset + sizeof(int));
    fseek(m_file, offset, SEEK_SET);
    fwrite(reinterpret_cast<char*>(&i), 1, sizeof(int), m_file);
}

void DataBase::write_string(size_t offset, const std::string& str)
{
    check_size(offset + str.size());
    fseek(m_file, offset, SEEK_SET);
    fwrite(str.data(), 1, str.size(), m_file);
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
    // NOTE: There can only be 1 table for now
    assert (!m_table);

    m_table = new Table(*this, constructor);
    return *m_table;
}

std::optional<Table> DataBase::get_table(const std::string &name)
{
    if (!m_table)
        return std::nullopt;

    return *m_table;
}

DataBase::~DataBase()
{
    if (m_file)
    {
        if (m_table)
            delete m_table;

        fclose(m_file);
    }
}
