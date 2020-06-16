#pragma once
#include "forward.hpp"
#include "allocator.hpp"
#include <array>
#include <assert.h>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>
#include <memory>

#ifdef DEBUG_ALLOCATOR
#include <cxxabi.h>
#include <iostream>
#endif

namespace Json
{

    class AllocatorCustom : public Allocator
    {
        friend Value;
        friend Object;
        friend Array;
        friend Document;

    public:
        AllocatorCustom();
        
        virtual Null *make_null() override;
        virtual Object *make_object() override;
        virtual Array *make_array() override;
        virtual String *make_string(const std::string_view) override;
        virtual Number *make_number(double) override;
        virtual Boolean *make_boolean(bool) override;
        
#ifdef DEBUG_ALLOCATOR
        virtual void report_usage() override;
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
