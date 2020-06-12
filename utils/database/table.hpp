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
        class Constructor
        {
        public:
            Constructor(std::string name);

            void add_column(std::string name, DataType type)
            {
                m_columns.emplace_back(name, type);
            }
            
        private:
            std::string m_name;
            std::vector<std::pair<std::string, DataType>> m_columns;

        };
        
    private:
        Table(DataBase &db, size_t header_offset)
            : m_db(db)
            , m_header_offset(header_offset) {}
        
        Table(DataBase&, Constructor);

        DataBase &m_db;
        size_t m_header_offset;
        std::vector<Column> m_columns;
        
    };
    
}
