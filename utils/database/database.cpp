#include "database.hpp"
#include <cassert>
using namespace DB;

std::optional<DataBase> DataBase::load_from_file(const std::string &path)
{
    // TODO: Implement this
    assert (false);
}

std::optional<DataBase> DataBase::create_new()
{
    return DataBase();
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
