#pragma once
#include "forward.hpp"
#include <memory>

namespace DB
{
    
    class Cell
    {
    public:
        enum Type
        {
            Integer,
        };
        
        static std::unique_ptr<Cell> make_integer(int);
      
    protected:
        Cell(Type type)
            : m_type(type) {}
        
    private:
        Type m_type;
    };
    
    class CellInteger : public Cell
    {
    public:
        CellInteger(int i)
            : Cell(Integer)
            , m_i(i) {}

        inline int data() const { return m_i; }

    private:
        int m_i;
        
    };
    
    class Column
    {
        friend Table;
        
    public:
        inline const std::string &name() const { return m_name; }
        inline Cell::Type data_type() const { return m_data_type; }
        
    private:
        Column(std::string name, Cell::Type data_type)
            : m_name(name)
            , m_data_type(data_type) {}
        
        std::string m_name;
        Cell::Type m_data_type;
        
    };
    
}
