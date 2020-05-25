#include "libjson.hpp"
#include <iostream>
#include <fstream>

int main()
{
    auto doc = Json::Document::parse(std::ifstream("test.json"));
    auto root = doc.root();
    if (doc.has_error())
        doc.log_errors();
    
    if (root)
        std::cout << root->to_string(Json::PrintOption::PrettyPrint) << "\n";
    return 0;
}
