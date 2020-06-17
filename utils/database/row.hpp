#pragma once
#include "column.hpp"
#include "forward.hpp"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace DB
{

    class Row
    {
        friend Table;
        friend Sql::SelectStatement;

    public:
        class const_itorator
        {
            friend Row;
            
        public:
            void operator++ () { m_index += 1; }
            bool operator== (const const_itorator &other) const { return m_index == other.m_index; }
            bool operator!= (const const_itorator &other) const { return !(*this == other); }
            std::pair<std::string, const Entry*> operator*() const
            {
                auto &entitiy = m_row.m_entities[m_index];
                return std::make_pair(entitiy.column.name(), entitiy.entry.get());
            }
            
        private:
            const_itorator(const Row &row, int index)
                : m_row(row)
                , m_index(index) {}
            
            const Row &m_row;
            int m_index;
        };
        
        const auto begin() const { return const_itorator(*this, 0); }
        const auto end() const { return const_itorator(*this, m_entities.size()); }
        std::unique_ptr<Entry> &operator [](const std::string &name);

        void read(Chunk &chunk, size_t row_offset);
        void write(Chunk &chunk, size_t row_offset);
        
    private:
        explicit Row(const std::vector<Column> &columns);

        // Create a row based of a selection
        explicit Row(std::vector<std::string> select_columns, Row &&other);

        struct Entity
        {
            Column column;
            size_t offset_in_row;
            std::unique_ptr<Entry> entry;
        };
        std::vector<Entity> m_entities;
    };

}

std::ostream &operator<<(std::ostream&, const DB::Row&);
