#pragma once
#include <string>
#include <vector>
#include <optional>

namespace DB
{

    class Cleaner
    {
    public:
        Cleaner(const std::string &path);

        void output_info();
        void full_clean_up();

    private:
        struct Chunk
        {
            size_t offset;

            char type[2];
            uint8_t owner_id;
            uint8_t index;
            size_t size_in_bytes;
            size_t padding_in_bytes;
        };

        struct Table
        {
            Chunk header;
            std::vector<Chunk> row_data;
            std::vector<Chunk> dynamic;
        };

        void process_data_base();
        
        std::string m_in_path;
        std::string m_out_path;
        
        bool m_has_been_processed { false };
        std::vector<Table> m_tables;
        std::optional<Chunk> m_version;

    };

}
