#pragma once
#include "table.hpp"
#include <optional>
#include <string>

namespace DB
{
    
    class DataBase
    {
    public:
        ~DataBase();
        
        static std::optional<DataBase> open(const std::string &path);
        
        Table &construct_table(Table::Constructor);
        
    private:
        DataBase();

        // NOTE: We can only have 1 table for now
        Table *m_table { nullptr };
        
    };
    
}
