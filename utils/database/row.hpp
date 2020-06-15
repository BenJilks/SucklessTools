#pragma once
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
        class Constructor
        {
            friend Row;
            friend Table;

        public:
            void integer_entry(int);

        private:
            Constructor(Chunk &chunk, size_t row_offset);

            Chunk &m_chunk;
            size_t m_row_offset;
            size_t m_curr_entry_offset { 0 };
            std::vector<std::unique_ptr<Entry>> m_entries;
        };

        const auto begin() const { return m_entries.begin(); }
        const auto end() const { return m_entries.end(); }
        Entry &operator [](const std::string &name);

    private:
        explicit Row(Constructor&& constructor, const std::vector<Column> &columns);
        explicit Row(Chunk &chunk)
            : m_chunk(chunk) {}

        // Create a row based of a selection
        explicit Row(std::vector<std::string> select_columns, Row &&other);

        Chunk &m_chunk;
        std::unordered_map<std::string, std::unique_ptr<Entry>> m_entries;
    };

}

std::ostream &operator<<(std::ostream&, const DB::Row&);
