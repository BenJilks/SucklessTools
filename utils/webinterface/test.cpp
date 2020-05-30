#include "webinterface.hpp"
#include <iostream>

int main()
{
    Web::Interface test;

    test.route("/", [](const auto& url, auto &response)
    {
        response.send_template("test.html", "name=Bob");
    });

    test.route("/test", [](const auto&, auto &response)
    {
    });

    test.start();
    return 0;
}
