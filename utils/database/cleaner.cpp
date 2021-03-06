#include "config.hpp"
#include "cleaner.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory.h>
#include <algorithm>
#include <optional>
using namespace DB;

Cleaner::Cleaner(const std::string &path)
    : m_in_path(path)
{
    int last_dot_index = 0;
    for (int i = path.size() - 1; i >= 0; i--)
    {
        if (path[i] == '.')
        {
            last_dot_index = i;
            break;
        }
    }

    m_out_path = path.substr(0, last_dot_index);
    m_out_path += "_cleaned";
    m_out_path += path.substr(last_dot_index, path.size());
}

static void read_int(std::istream &in, size_t &out)
{
    out = 0;
    out |= in.get() << 8*0;
    out |= in.get() << 8*1;
    out |= in.get() << 8*2;
    out |= in.get() << 8*3;
}

void Cleaner::output_info()
{
    if (!m_has_been_processed)
        process_data_base();
    
    auto print_chunk = [&](const Chunk &chunk)
    {
        std::cout << "Chunk "
            << "type = " << std::string_view(chunk.type, 2) << ", "
            << "owner_id = " << (int)chunk.owner_id << ", "
            << "index = " << (int)chunk.index << ", "
            << "size = " << chunk.size_in_bytes << ", "
            << "padding = " << chunk.padding_in_bytes << "\n";
    };

    if (m_version)
        print_chunk(*m_version);
    
    for (const auto &table : m_tables)
    {
        print_chunk(table.header);
        
        std::cout << "\tRow Data:\n";
        for (const auto &chunk : table.row_data)
        {
            std::cout << "\t\t";
            print_chunk(chunk);
        }
        
        std::cout << "\tDynamic Data:\n";
        for (const auto &chunk : table.dynamic)
        {
            std::cout << "\t\t";
            print_chunk(chunk);
        }
    }
}

void Cleaner::process_data_base()
{
    std::ifstream in(m_in_path, std::ifstream::binary);
    auto find_table = [&](uint8_t id) -> Table&
    {
        for (auto &table : m_tables)
        {
            if (table.header.owner_id == id)
                return table;
        }

        assert (false);
    };
    
    size_t index = 0;
    for (;;)
    {
        char c = in.get();
        if (c == (char)EOF)
            break;

        Chunk chunk;
        chunk.offset = index;
        chunk.type[0] = c;
        chunk.type[1] = in.get();
        chunk.owner_id = in.get();
        chunk.index = in.get();
        read_int(in, chunk.size_in_bytes);
        read_int(in, chunk.padding_in_bytes);

        auto type_str = std::string_view(chunk.type, 2);
        if (type_str == "VR")
            m_version = chunk;
        else if (type_str == "TH")
            m_tables.push_back({ chunk });
        else if (type_str == "RD")
            find_table(chunk.owner_id).row_data.push_back(chunk);
        else if (type_str == "DY")
            find_table(chunk.owner_id).dynamic.push_back(chunk);

        index += Config::chunk_header_size;
        index += chunk.size_in_bytes;
        index += chunk.padding_in_bytes;
        in.seekg(index);
    }

    m_has_been_processed = true;
}

void Cleaner::full_clean_up()
{
    if (!m_has_been_processed)
        process_data_base();
    
    std::ifstream in(m_in_path, std::ifstream::binary);
    std::ofstream out(m_out_path, std::ifstream::binary);
    auto write_chunk_header = [&](const Chunk &chunk)
    {
        auto write_int = [&](int i)
        {
            out.write(((char*)&i) + 0, 1);
            out.write(((char*)&i) + 1, 1);
            out.write(((char*)&i) + 2, 1);
            out.write(((char*)&i) + 3, 1);
        };

        out.write(chunk.type, 2);
        out.write((char*)&chunk.owner_id, 1);
        out.write((char*)&chunk.index, 1);
        write_int(chunk.size_in_bytes);
        write_int(0); // NOTE: We ignore the padding
        for (int i = 0; i < Config::chunk_header_size - 12; i++)
            out.write("\0", 1);
   };

    auto copy_chunk_body = [&](const Chunk &chunk, bool is_row_data = false)
    {
        in.clear();
        in.seekg(chunk.offset + Config::chunk_header_size, std::ifstream::beg);

        auto count = ((int)chunk.size_in_bytes) - (is_row_data ? 1 : 0);
        for (int i = 0; i < count; i++)
        {
            char c = in.get();
            out.write(&c, 1);
        }
    };

    if (m_version)
    {
        write_chunk_header(*m_version);
        copy_chunk_body(*m_version);
    }

    for (auto &table : m_tables)
    {
        auto sort_chunks = [&](auto collection)
        {
            std::sort(collection.begin(), collection.end(),
                [&](const auto &a, const auto &b)
            {
                return a.index < b.index;
            });
        };

        // Write table header and sort sub-chunks
        write_chunk_header(table.header);
        copy_chunk_body(table.header);
        sort_chunks(table.row_data);
        sort_chunks(table.dynamic);

        // Create new coallated row data chunk
        Chunk coallated_row_data;
        coallated_row_data.type[0] = 'R';
        coallated_row_data.type[1] = 'D';
        coallated_row_data.owner_id = table.header.owner_id;
        coallated_row_data.index = 0;
        coallated_row_data.size_in_bytes = 0;
        for (const auto chunk : table.row_data)
            coallated_row_data.size_in_bytes += chunk.size_in_bytes - 1;
        coallated_row_data.padding_in_bytes = 0;

        // Write row data to new chunk
        write_chunk_header(coallated_row_data);
        for (const auto &chunk : table.row_data)
            copy_chunk_body(chunk, true);

        // Write dynamic chunks in order
        for (const auto &chunk : table.dynamic)
        {
            write_chunk_header(chunk);
            copy_chunk_body(chunk);
        }
    }
}
