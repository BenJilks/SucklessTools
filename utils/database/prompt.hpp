#pragma once
#include "forward.hpp"
#include <string>
#include <memory>

namespace DB
{
    
    class Prompt
    {
    public:
        Prompt(const std::string &database_path);
        void run();
        
    private:
        std::shared_ptr<DataBase> m_db;
    
    };
    
}
