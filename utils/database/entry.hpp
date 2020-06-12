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
            Integer,
        };
        
        DataType(Primitive primitive, size_t size, size_t length)
            : m_primitive(primitive)
            , m_size(size)
            , m_length(length) {}
        
        static DataType integer();
        
    private:
        Primitive m_primitive;
        size_t m_size;
        size_t m_length;
        
    };
    
    class Entry
    {
        friend Column;
        
    protected:
        Entry(DataBase &db, DataType type, size_t data_offset)
            : m_db(db)
            , m_type(type)
            , m_data_offset(data_offset) {}
        
    private:
        DataBase &m_db;
        DataType m_type;
        size_t m_data_offset;
        
    };
    
    class IntegerEntry : public Entry
    {
        friend Column;
        
    public:
        inline int data() const { return m_i; }

    private:
        IntegerEntry(DataBase &db, size_t data_offset, int i)
            : Entry(db, DataType::integer(), data_offset)
            , m_i(i) {}

        int m_i;
        
    };

}
