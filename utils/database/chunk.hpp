#pragma once
#include "forward.hpp"
#include <string>

namespace DB
{

    class Chunk
    {
        friend DataBase;
        friend DynamicData;

    public:
        Chunk(const Chunk&) = delete;
        Chunk(Chunk&) = delete;

        inline std::string_view type() const { return std::string_view(m_type, 2); }
        inline size_t data_offset() const { return m_data_offset; }
        inline size_t size_in_bytes() const { return m_size_in_bytes; }
        inline size_t padding_in_bytes() const { return m_padding_in_bytes; }
        inline size_t owner_id() const { return m_owner_id; }
        inline size_t index() const { return m_index; }
        inline void increment_index(int by) { m_index += by; }
        inline DataBase &db() { return m_db; }
        size_t header_size() const;
        bool is_active() const;

        uint8_t read_byte(size_t offset);
        int read_int(size_t offset);
        int64_t read_long(size_t offset);
        std::string read_string(size_t offset, size_t len);

        void write_byte(size_t offset, uint8_t);
        void write_int(size_t offset, int);
        void write_long(size_t offset, int64_t);
        void write_string(size_t offset, const std::string&);
        void shrink_to(size_t offset);
        void drop();

    protected:
        Chunk(Chunk&&) = default;

    private:
        Chunk(DataBase&, size_t header_offset);
        Chunk(DataBase& db)
            : m_db(db) {}

        void check_size(size_t size);

        DataBase &m_db;
        size_t m_header_offset;
        size_t m_data_offset;

        char m_type[2];
        size_t m_size_in_bytes { 0 };
        size_t m_padding_in_bytes { 0 };
        uint8_t m_owner_id { 0xCD };
        uint8_t m_index { 0xCD };
        bool m_has_been_dropped { false };

    };

}
