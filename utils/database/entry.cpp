#include "entry.hpp"
using namespace DB;

DataType DataType::integer()
{
    return DataType(Integer, 4, 1);
}
