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
        
        static std::optional<DataBase> load_from_file(const std::string &path);
        static std::optional<DataBase> create_new();
        
        Table &construct_table(Table::Constructor);
        
    private:
        DataBase();

        // NOTE: We can only have 1 table for now
        Table *m_table { nullptr };
        
    };
    
}
