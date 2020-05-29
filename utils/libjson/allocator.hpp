#pragma once
#include "forward.hpp"
#include <array>
#include <assert.h>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>

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

        String *make_string_from_buffer(const std::string &buffer);

        template<typename T, typename ...Args>
        T *make(Args ...args)
        {
            insure_size(sizeof(T));

            auto location = m_current_memory_chunk->memory.get() + m_current_memory_chunk->pointer;
            auto ptr = new (location) T(*this, args...);
            m_current_memory_chunk->pointer += sizeof(T);

#ifdef DEBUG_ALLOCATOR
            std::cout << "Allocator: Made " << abi::__cxa_demangle(typeid(T).name(), 0, 0, nullptr)
                    << " at " << (void*)ptr << ", of size: " << sizeof(T) << "\n";
#endif
            return ptr;
        }

        void insure_size(size_t);

        std::shared_ptr<MemoryChunk> m_memory_chain;
        MemoryChunk *m_current_memory_chunk;
        static constexpr size_t s_link_size = 1024;
    };

}
