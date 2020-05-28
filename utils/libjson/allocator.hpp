#pragma once
#include <sys/types.h>
#include <vector>
#include <string>
#include <array>
#include <optional>

//#define DEBUG_ALLOCATOR

#ifdef DEBUG_ALLOCATOR
#include <cxxabi.h>
#include <iostream>
#endif

namespace Json
{

    class Value;
    class String;

    template<typename T>
    class Owner;

    template<typename T>
    class WeakRef
    {
        friend Owner<T>;

    public:
        WeakRef() = delete;
        WeakRef(WeakRef &other) : m_data(other.m_data) {}
        WeakRef(const WeakRef &other) : m_data(other.m_data) {}

        T &operator*() { return *m_data; }
        T *operator->() { return m_data; }
        const T &operator*() const { return *m_data; }
        const T *operator->() const { return m_data; }

    private:
        explicit WeakRef(T *data)
            : m_data(data) {}

        T *m_data { nullptr };

    };

    template<typename T>
    class Owner
    {
        template<class U>
        friend class Owner;

    public:
        Owner()
            : m_data(nullptr) {}

        Owner(Owner<T> &other) = delete;
        Owner(const Owner<T> &other) = delete;
        Owner(const Owner<T> &&other) = delete;

        Owner(T *data, std::function<void(T* t)> deleter)
            : m_data(data)
            , m_deleter(deleter) {}

        // Move operation
        Owner(Owner<T> &&other)
            : m_data(other.m_data)
            , m_deleter(other.m_deleter)
        {
            other.m_data = nullptr;
        }

        template<typename U>
        Owner(Owner<U> &&other)
            : m_data(static_cast<T*>(other.m_data))
            , m_deleter(std::bind(other.m_deleter, static_cast<U*>(m_data)))
        {
            other.m_data = nullptr;
        }

        Owner<T>& operator=(Owner<T>&& other) noexcept
        {
            m_data = other.m_data;
            m_deleter = other.m_deleter;
            other.m_data = nullptr;
            return *this;
        }

        T &operator*() { return *m_data; }
        T *operator->() { return m_data; }
        const T &operator*() const { return *m_data; }
        const T *operator->() const { return m_data; }

        operator bool() { return m_data; }
        operator bool() const { return m_data; }

        WeakRef<T> make_weak_ref() { return WeakRef<T>(m_data); }
        const WeakRef<T> make_weak_ref() const { return WeakRef<T>(m_data); }

        ~Owner()
        {
            if (m_data)
            {
                if (m_deleter)
                    m_deleter(m_data);
                else
                    delete m_data;
            }
        }

    private:
        T *m_data;
        std::function<void(T* t)> m_deleter;

    };

    class Allocator
    {
    public:
        Allocator();

        struct MemoryChunk
        {
            MemoryChunk(int size)
                : size(size)
                , memory(std::unique_ptr<char[]>(new char[size])) {}

            ~MemoryChunk();

            int size;
            int pointer { 0 };
            std::unique_ptr<char[]> memory { nullptr };
        };

        Owner<String> make_string_from_buffer(const std::string &buffer);

        template<typename T, typename ...Args>
        Owner<T> make(Args ...args)
        {
            insure_size(sizeof(T));

            auto location = m_current_memory_link->memory.get() + m_current_memory_link->pointer;
            auto ptr = new (location) T(m_current_memory_link, args...);
            m_current_memory_link->pointer += sizeof(T);

#ifdef DEBUG_ALLOCATOR
    std::cout << "Allocator: Made " << abi::__cxa_demangle(typeid(T).name(), 0, 0, nullptr)
              << " at " << ptr << ", of size: " << sizeof(T) << "\n";
#endif
            return Owner<T>(ptr, [](T* t)
            {
#ifdef DEBUG_ALLOCATOR
    std::cout << "Allocator: Free " << abi::__cxa_demangle(typeid(T).name(), 0, 0, nullptr) << "\n";
#endif
                t->~T();
            });
        }

    private:
        void insure_size(size_t);

        std::shared_ptr<MemoryChunk> m_current_memory_link;
        static constexpr size_t s_link_size = 1024;
    };

}
