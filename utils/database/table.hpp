#pragma once
#include "forward.hpp"
#include "column.hpp"
#include "row.hpp"
#include <vector>
#include <string>
#include <optional>

namespace DB
{

    class Table
    {
        friend DataBase;

    public:
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

        Row::Constructor new_row();
        std::optional<Row> add_row(Row::Constructor);
        std::optional<Row> get_row(size_t index);

    private:
        Table(DataBase&, Constructor);
        Table(std::shared_ptr<Chunk> header);

        void add_row_data(std::shared_ptr<Chunk> data);
        void write_header();

        std::shared_ptr<Chunk> m_header;
        std::shared_ptr<Chunk> m_data;
        size_t m_row_count_offset;

        int m_id { 0xCD };
        std::string m_name;
        std::vector<Column> m_columns;
        size_t m_row_size { 0 };
        size_t m_row_count { 0 };

    };

}
