#pragma once

#include <functional>
#include "item.hpp"

class Output
{
public:
    virtual int run() = 0;

    std::function<ItemList(const std::string &name, int count)> on_request_items;

protected:
    Output() = default;

};
