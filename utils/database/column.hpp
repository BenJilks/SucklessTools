#pragma once
#include "forward.hpp"
#include "entry.hpp"
#include <memory>
#include <string>

namespace DB
{

    class Column
    {
        friend Table;

    public:
        inline const std::string &name() const { return m_name; }
        inline DataType data_type() const { return m_data_type; }

    private:
        Column(std::string name, DataType data_type)
            : m_name(name)
            , m_data_type(data_type) {}

        std::unique_ptr<Entry> read(Chunk &chunk, size_t offset) const;

        std::string m_name;
        DataType m_data_type;

    };

}
