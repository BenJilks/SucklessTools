#include <iostream>
#include "xlib.hpp"

int main()
{
    XLibOutput output;
    output.on_request_items = [&](auto&, int count)
    {
        ItemList item_list;
        for (int i = 0; i < count; i++)
            item_list.add(Item("Test" + std::to_string(i)));

        return item_list;
    };

    return output.run();
}
