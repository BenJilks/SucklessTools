#pragma once
#include "forward.hpp"
#include <vector>

namespace DB
{

    class DynamicData
    {
    public:
        DynamicData(std::shared_ptr<Chunk> chunk)
            : m_chunk(chunk) {}

        int id() const;
        void set(const std::vector<char> &data);
        std::vector<char> read();

    private:
        std::shared_ptr<Chunk> m_chunk;

    };

}
