#pragma once
#include "table.hpp"
#include "sql/sql.hpp"
#include <iostream>
#include <optional>
#include <string>

namespace DB
{

    class Chunk
    {
        friend DataBase;

    public:
        Chunk(const Chunk&) = delete;
        Chunk(Chunk&) = delete;
        Chunk(Chunk&&) = delete;

        inline std::string_view type() const { return std::string_view(m_type, 2); }
        inline size_t data_offset() const { return m_data_offset; }
        inline size_t size_in_bytes() const { return m_size_in_bytes; }
        inline size_t padding_in_bytes() const { return m_padding_in_bytes; }
        inline size_t owner_id() const { return m_owner_id; }
        inline size_t index() const { return m_index; }
        inline size_t header_size() const { return 20; }
        inline void increment_index(int by) { m_index += by; }
        bool is_active() const;

        uint8_t read_byte(size_t offset);
        int read_int(size_t offset);
        std::string read_string(size_t offset, size_t len);

        void write_byte(size_t offset, uint8_t);
        void write_int(size_t offset, int);
        void write_string(size_t offset, const std::string&);
        void drop();
        void shrink_to(size_t offset);

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

    class DataBase
    {
        friend Chunk;
        friend Table;
        friend IntegerEntry;

    public:
        ~DataBase();

        DataBase() = delete;
        DataBase(const DataBase&) = delete;
        DataBase(DataBase&) = delete;

        static std::shared_ptr<DataBase> open(const std::string &path);

        Table &construct_table(Table::Constructor);
        Table *get_table(const std::string &name);
        bool drop_table(const std::string &name);

        SqlResult execute_sql(const std::string &query);

    private:
        explicit DataBase(FILE *file);

        std::shared_ptr<Chunk> new_chunk(std::string_view type, uint8_t owner_id, uint8_t index);
        void check_is_active_chunk(Chunk *chunk);
        uint8_t generate_table_id();

        void check_size(size_t);
        void write_byte(size_t offset, char);
        void write_int(size_t offset, int);
        void write_string(size_t offset, const std::string&);
        void flush();

        uint8_t read_byte(size_t offset);
        int read_int(size_t offset);
        void read_string(size_t offset, char *str, size_t len);

        FILE *m_file;
        size_t m_end_of_data_pointer;

        std::vector<Table> m_tables;
        std::vector<std::shared_ptr<Chunk>> m_chunks;
        std::shared_ptr<Chunk> m_active_chunk { nullptr };

    };

}

std::ostream &operator <<(std::ostream&, const DB::Chunk&);
