#pragma once
#include <optional>
#include <string>

namespace DB
{
    
    class DataBase
    {
    public:
        std::optional<DataBase> load_from_file(const std::string &path);
        std::optional<DataBase> create_new();
        
    private:
        DataBase();
        
    };
    
}
