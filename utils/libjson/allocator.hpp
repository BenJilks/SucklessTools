#pragma once
#include "forward.hpp"
#include <array>
#include <assert.h>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>
#include <memory>

//#define DEBUG_ALLOCATOR

#ifdef DEBUG_ALLOCATOR
#include <cxxabi.h>
#include <iostream>
#endif

namespace Json
{

    class Allocator
    {
        friend Value;
        friend Object;
        friend Array;
        friend Document;

    public:
        Allocator();
        
#ifdef DEBUG_ALLOCATOR
        void report_usage();
#endif

    private:
        struct MemoryChunk
        {
            MemoryChunk(int size)
                : size(size)
                , memory(std::unique_ptr<char[]>(new char[size])) {}

            ~MemoryChunk();

            int size;
            int pointer { 0 };
            std::unique_ptr<char[]> memory { nullptr };
            std::unique_ptr<MemoryChunk> next { nullptr };
        };

        String *make_string_from_buffer(const std::string_view buffer);

        template<typename T, typename ...Args>
        T *make(Args ...args)
        {
            insure_size(sizeof(T));

            auto location = m_current_memory_chunk->memory.get() + m_current_memory_chunk->pointer;
            auto ptr = new (location) T(*this, args...);
            m_current_memory_chunk->pointer += sizeof(T);

#ifdef DEBUG_ALLOCATOR
            m_objects_allocated += 1;
            m_objects_in_use += 1;
            m_max_usage += sizeof(T);
            std::cout << "Allocator: Made " << abi::__cxa_demangle(typeid(T).name(), 0, 0, nullptr)
                    << " at " << (void*)ptr << ", of size: " << sizeof(T) << "\n";
#endif
            return ptr;
        }

        void insure_size(size_t);
        void did_delete();

        std::unique_ptr<MemoryChunk> m_memory_chain;
        MemoryChunk *m_current_memory_chunk { nullptr };
        static constexpr size_t s_link_size = 1024;
        
#ifdef DEBUG_ALLOCATOR
        // Memory tracking data
        size_t m_max_usage { 0 };
        size_t m_objects_allocated { 0 };
        size_t m_objects_in_use { 0 };
        size_t m_strings_allocated { 0 };
#endif
    };

}
