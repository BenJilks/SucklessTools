#include "column.hpp"
using namespace DB;

std::unique_ptr<Cell> Cell::make_integer(int i)
{
    return std::make_unique<CellInteger>(i);
}
