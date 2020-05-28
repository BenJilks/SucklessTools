#include "libjson.hpp"
#include <fstream>
#include <iostream>

static int alloc_count = 0;
void* operator new(size_t size)
{
    alloc_count += 1;
    return malloc(size);
}

int main()
{
    Json::Owner<int> i(new int(21), [](int *i) { delete i; });
    auto other = std::move(i);

    auto doc = Json::Document::parse(std::ifstream("test.json"));
    auto &root = doc.root();
    if (doc.has_error())
        doc.log_errors();
    std::cout << root.to_string(Json::PrintOption::PrettyPrint) << "\n";
    std::cout << "Made " << alloc_count << " allocations\n";
    return 0;
}
