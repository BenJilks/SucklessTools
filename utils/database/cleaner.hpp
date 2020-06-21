#pragma once
#include <string>

namespace DB
{

    class Cleaner
    {
    public:
        Cleaner(const std::string &path);
        void full_clean_up();

    private:
        std::string m_in_path;
        std::string m_out_path;

    };

}
