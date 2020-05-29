#include "libjson.hpp"
#include <fstream>
#include <iostream>

static int alloc_count = 0;
void* operator new(size_t size)
{
    std::cout << "Allocate " << size << "\n";
    alloc_count += 1;
    return malloc(size);
}

int main()
{
    auto doc = Json::Document::parse(std::ifstream("test.json"));
    auto &root = doc.root();
    if (doc.has_error())
        doc.log_errors();
    std::cout << root.to_string(Json::PrintOption::PrettyPrint) << "\n";
    std::cout << "Made " << alloc_count << " allocations\n";

    return 0;
}
