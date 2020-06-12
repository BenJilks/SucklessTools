#pragma once
#include "forward.hpp"
#include "column.hpp"
#include <vector>

namespace DB
{
    
    class Table
    {
        friend DataBase;
        
    public:
        
    private:
        Table(DataBase &database, size_t header_offset)
            : m_database(database)
            , m_header_offset(header_offset) {}
        
        DataBase &m_database;
        size_t m_header_offset;
        std::vector<Column> m_columns;
        
    };
    
}
