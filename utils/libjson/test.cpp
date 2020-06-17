#include "libjson.hpp"
#include <fstream>
#include <iostream>

int main()
{
    auto test = Json::parse(std::ifstream("test.json"));
    std::cout << test << "\n";
    return 0;
}
