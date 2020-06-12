#include "database.hpp"
#include <iostream>
#include <cassert>
using namespace DB;

int main()
{
	std::cout << "Hello, databases!!!\n";

    auto db_or_error = DataBase::open("test.db");
    assert (db_or_error);
    
    auto db = *db_or_error;
    
    Table::Constructor tc("Test");
    tc.add_column("ID", DataType::integer());
    tc.add_column("FirstName", DataType::integer());
    tc.add_column("LastName", DataType::integer());
    tc.add_column("Age", DataType::integer());
    
    auto &table = db.construct_table(tc);
    std::cout << "Made table " << &table << "\n";
    
    return 0;
}
