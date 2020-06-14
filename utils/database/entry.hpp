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
        Entry(Chunk &chunk, DataType type, size_t data_offset)
            : m_chunk(chunk)
            , m_type(type)
            , m_data_offset(data_offset) {}
        virtual ~Entry() = default;

        const DataType &type() const { return m_type; }
        virtual void write() = 0;

    protected:
        inline Chunk &chunk() { return m_chunk; }
        inline size_t &data_offset() { return m_data_offset; }

    private:
        Chunk &m_chunk;
        DataType m_type;
        size_t m_data_offset;

    };

    class IntegerEntry : public Entry
    {
        friend Column;

    public:
        IntegerEntry(Chunk &chunk, size_t data_offset);
        IntegerEntry(Chunk &chunk, size_t data_offset, int i)
            : Entry(chunk, DataType::integer(), data_offset)
            , m_i(i) {}

        inline int data() const { return m_i; }
        virtual void write() override;

    private:
        int m_i;

    };

}

std::ostream &operator<< (std::ostream&, const DB::Entry&);
