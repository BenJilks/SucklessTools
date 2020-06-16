#pragma once
#include "forward.hpp"
#include <memory>

namespace DB
{

    class DataType
    {
    public:
        enum Primitive
        {
            Integer = 0,
        };

        DataType(Primitive primitive, size_t size, size_t length)
            : m_primitive(primitive)
            , m_size(size)
            , m_length(length) {}

        static DataType integer();

        inline Primitive primitive() const { return m_primitive; }
        inline size_t size() const { return m_size; }
        inline size_t length() const { return m_length; }

        bool operator== (const DataType &other) const;
        bool operator!= (const DataType &other) const { return !(*this == other); }

    private:
        Primitive m_primitive;
        size_t m_size;
        size_t m_length;

    };

    class Entry
    {
        friend Column;

    public:
        Entry(DataType type)
            : m_type(type) {}
        virtual ~Entry() = default;

        const DataType &type() const { return m_type; }
        virtual void read(Chunk &chunk, size_t offset) = 0;
        virtual void write(Chunk &chunk, size_t offset) = 0;

    private:
        DataType m_type;

    };

    class IntegerEntry : public Entry
    {
        friend Column;

    public:
        IntegerEntry(Chunk &chunk, size_t offset)
            : Entry(DataType::integer())
        {
            read(chunk, offset);
        }
        
        IntegerEntry(int i)
            : Entry(DataType::integer())
            , m_i(i) {}

        inline int data() const { return m_i; }
        virtual void read(Chunk &chunk, size_t offset) override;
        virtual void write(Chunk &chunk, size_t offset) override;

    private:
        int m_i;

    };

}

std::ostream &operator<< (std::ostream&, const DB::Entry&);
