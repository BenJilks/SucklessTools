#include "allocator.hpp"
#include <iostream>
#include <algorithm>
#include "libjson.hpp"
using namespace Json;

#define BUFFER_STEPS 1024

Allocator::Allocator()
{
}


Allocator::MemoryChunk::~MemoryChunk()
{
#ifdef DEBUG_ALLOCATOR
    std::cout << "Allocator: free chunk of size " << size << "\n";
#endif
}

void Allocator::insure_size(size_t size)
{
    // If the memory chain is empty, make an initial one
    if (!m_current_memory_link)
    {
        m_current_memory_link = std::make_shared<MemoryChunk>(std::max(s_link_size, size));
#ifdef DEBUG_ALLOCATOR
    std::cout << "Allocator: initial chunk " << m_current_memory_link->size << "\n";
#endif
    }

    // If this new allocation will go outside the current memory link's range, then make a
    // new one, insuring that this new data will fit
    if (m_current_memory_link->pointer + size > m_current_memory_link->size)
    {
        m_current_memory_link = std::make_shared<MemoryChunk>(std::max(s_link_size, size));
#ifdef DEBUG_ALLOCATOR
    std::cout << "Allocator: further chunk " << m_current_memory_link->size << "\n";
#endif
    }
}

Owner<String> Allocator::make_string_from_buffer(const std::string &buffer)
{
    insure_size(buffer.size());

    auto ptr = m_current_memory_link->memory.get() + m_current_memory_link->pointer;
    memcpy(ptr, buffer.data(), buffer.size());
    m_current_memory_link->pointer += buffer.size();

#ifdef DEBUG_ALLOCATOR
    std::cout << "Allocator: Copy string data to " << (void*)ptr << ", of size " << buffer.size() << "\n";
#endif

    auto str = std::string_view(ptr, buffer.size());
    return make<String>(str);
}
