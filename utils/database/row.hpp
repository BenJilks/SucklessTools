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
        const auto begin() const { return m_entries.begin(); }
        const auto end() const { return m_entries.end(); }
        std::unique_ptr<Entry> &operator [](const std::string &name);

    private:
        explicit Row(const std::vector<Column> &columns);

        // Create a row based of a selection
        explicit Row(std::vector<std::string> select_columns, Row &&other);

        std::unordered_map<std::string, std::unique_ptr<Entry>> m_entries;
    };

}

std::ostream &operator<<(std::ostream&, const DB::Row&);
