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
        inline size_t length() const { return m_length; }
        inline size_t size() const 
        { 
            // NOTE: All types start with an 'is null' flag
            return 1 + m_size; 
        }

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
        Entry(DataType type, bool is_null = false)
            : m_is_null(is_null)
            , m_type(type) {}
        virtual ~Entry() = default;

        const DataType &type() const { return m_type; }
        void read(Chunk &chunk, size_t offset);
        void write(Chunk &chunk, size_t offset);
        
        inline bool is_null() const { return m_is_null; }
    
    protected:
        bool m_is_null;        
        
    private:
        virtual void read_data(Chunk &chunk, size_t offset) = 0;
        virtual void write_data(Chunk &chunk, size_t offset) = 0;

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
        
        // Null constructor
        IntegerEntry()
            : Entry(DataType::integer(), true) {}

        inline int data() const { return m_i; }

    private:
        virtual void read_data(Chunk &chunk, size_t offset) override;
        virtual void write_data(Chunk &chunk, size_t offset) override;

        int m_i;

    };

}

std::ostream &operator<< (std::ostream&, const DB::Entry&);
