#pragma once
#include "table.hpp"
#include "sql/sql.hpp"
#include <iostream>
#include <optional>
#include <string>

namespace DB
{

    class DataBase
    {
        friend Chunk;
        friend DynamicData;
        friend Table;
        friend IntegerEntry;
        friend TextEntry;

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
        Table *find_owner(uint8_t owner_id);

        void check_size(size_t);
        void write_byte(size_t offset, char);
        void write_int(size_t offset, int);
        void write_long(size_t offset, int64_t);
        void write_string(size_t offset, const std::string&);
        void write_version_chunk();
        void flush();

        uint8_t read_byte(size_t offset);
        int read_int(size_t offset);
        int64_t read_long(size_t offset);
        void read_string(size_t offset, char *str, size_t len);

        FILE *m_file;
        size_t m_end_of_data_pointer;

        std::vector<Table> m_tables;
        std::vector<std::shared_ptr<Chunk>> m_chunks;
        std::shared_ptr<Chunk> m_active_chunk { nullptr };
        std::shared_ptr<Chunk> m_version_chunk { nullptr };

    };

}

std::ostream &operator <<(std::ostream&, const DB::Chunk&);
