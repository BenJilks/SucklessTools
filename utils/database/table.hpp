#pragma once
#include "forward.hpp"
#include "column.hpp"
#include "row.hpp"
#include <vector>
#include <string>
#include <optional>
#include <tuple>

namespace DB
{

    class Table
    {
        friend DataBase;

    public:
        Table(const Table&) = default;
        Table operator=(const Table&& table) { return Table(std::move(table)); };
        
        class Constructor
        {
            friend Table;

        public:
            Constructor(std::string name)
                : m_name(name) {}

            void add_column(std::string name, DataType type)
            {
                m_columns.emplace_back(name, type);
            }

        private:
            std::string m_name;
            std::vector<std::pair<std::string, DataType>> m_columns;

        };

        inline int id() const { return m_id; }
        inline const std::string &name() const { return m_name; }
        inline size_t row_count() const { return m_row_count; }

        std::optional<Row> get_row(size_t index);
        void add_row(Row);
        Row make_row();
        void drop();

    private:
        Table(DataBase&, Constructor);
        Table(DataBase&, std::shared_ptr<Chunk> header);

        std::tuple<std::shared_ptr<Chunk>, size_t> find_chunk_and_offset_for_row(size_t row);
        int find_next_row_chunk_index();
        void add_row_data(std::shared_ptr<Chunk> data);
        void write_header();

        DataBase &m_db;
        std::shared_ptr<Chunk> m_header;
        std::vector<std::shared_ptr<Chunk>> m_row_data_chunks;
        size_t m_row_count_offset;

        int m_id { 0xCD };
        std::string m_name;
        std::vector<Column> m_columns;
        size_t m_row_size { 0 };
        size_t m_row_count { 0 };

    };

}
