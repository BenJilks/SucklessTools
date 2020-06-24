#include "config.hpp"
#include "prompt.hpp"
#include "database.hpp"
#include <iostream>
using namespace DB;

Prompt::Prompt(const std::string &database_path)
{
    m_db = DataBase::open(database_path);
}

void Prompt::run()
{
    std::cout << "DataBase V" 
        << Config::major_version << "." << Config::minor_version
        << " prompt\n\n";
    
    for (;;)
    {
        std::cout << ">> ";
        
        std::string line;
        std::getline(std::cin, line);
        if (line == "exit")
            break;
        
        auto result = m_db->execute_sql(line);
        if (!result.good())
            result.output_errors();
        
        for (auto &row : result)
            std::cout << row << "\n";
        std::cout << "\n";
    }
}
