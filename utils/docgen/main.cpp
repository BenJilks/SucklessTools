#include <iostream>
#include <fstream>
#include "langs/cplusplus.hpp"

int main()
{
    std::cout << "Hello, DocGen!!\n";
    
    auto unit = CPP::Parser::parse("../main.cpp");
    unit.dump();
    return 0;
}
