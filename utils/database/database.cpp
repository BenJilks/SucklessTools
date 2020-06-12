#include "database.hpp"
#include <cassert>
using namespace DB;

std::optional<DataBase> DataBase::open(const std::string&)
{
    // TODO: Implement this
    assert (false);
}

Table &DataBase::construct_table(Table::Constructor constructor)
{
    // NOTE: There can only be 1 table for now
    assert (!m_table);
    
    m_table = new Table(*this, constructor);
}

DataBase::~DataBase()
{
    if (m_table)
        delete m_table;
}
