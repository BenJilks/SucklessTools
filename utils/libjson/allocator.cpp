#include "allocator.hpp"
#include "libjson.hpp"
#include <algorithm>
#include <iostream>
#include <cstring>
using namespace Json;

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
    if (!m_current_memory_chunk)
    {
        m_memory_chain = std::make_unique<MemoryChunk>(std::max(s_link_size, size));
        m_current_memory_chunk = m_memory_chain.get();
#ifdef DEBUG_ALLOCATOR
        std::cout << "Allocator: Create initial chunk " << m_current_memory_chunk->size << "\n";
#endif
    }

    // If this new allocation will go outside the current memory link's range, then make a
    // new one, insuring that this new data will fit
    if (m_current_memory_chunk->pointer + size > m_current_memory_chunk->size)
    {
        m_current_memory_chunk->next = std::make_unique<MemoryChunk>(std::max(s_link_size, size));
        m_current_memory_chunk = m_current_memory_chunk->next.get();
#ifdef DEBUG_ALLOCATOR
        std::cout << "Allocator: New chunk " << m_current_memory_chunk->size << "\n";
#endif
    }
}

void Allocator::did_delete()
{
#ifdef DEBUG_ALLOCATOR
    m_objects_in_use -= 1;
#endif
}

String *Allocator::make_string_from_buffer(const std::string_view buffer)
{
    insure_size(buffer.size());

    auto ptr = m_current_memory_chunk->memory.get() + m_current_memory_chunk->pointer;
    memcpy(ptr, buffer.data(), buffer.size());
    m_current_memory_chunk->pointer += buffer.size();

#ifdef DEBUG_ALLOCATOR
    m_strings_allocated += 1;
    m_max_usage += buffer.size();
    std::cout << "Allocator: Copy string data to " << (void*)ptr << ", of size " << buffer.size() << "\n";
#endif

    auto str = std::string_view(ptr, buffer.size());
    return make<String>(str);
}

#ifdef DEBUG_ALLOCATOR
void Allocator::report_usage()
{
    std::cout << "\nMemory usage report:\n";
    std::cout << "\tMax: " << m_max_usage << " bytes\n";
    std::cout << "\tObjects: " << m_objects_allocated << "\n";
    std::cout << "\tStrings: " << m_strings_allocated << "\n";
    std::cout << "\tObjects In Use: " << m_objects_in_use << "\n";
}
#endif
