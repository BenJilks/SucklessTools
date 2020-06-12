#include "webinterface.hpp"
#include "template.hpp"
#include "table.hpp"
#include <iostream>
#include <cassert>

int main()
{
    Web::Interface test;

    auto table_or_error = Web::Table::make("ID", "Test", "Test2");
    auto test_template_or_error = Web::Template::load_from_file("test.html");
    assert (test_template_or_error);
    assert (table_or_error);

    auto &test_template = *test_template_or_error;

    auto &table = *table_or_error;
    for (int i = 0; i < 21; i++)
    {
        auto &row = table.add_row();
        row["ID"] = std::to_string(i);
        row["Test"] = "lolz";
        row["Test2"] = "bob";
    }

    test.route("/", [&](const auto& url, auto &response)
    {
        test_template["name"] = "Bob";
        test_template["table"] = table.build();
        response.send_text(test_template.build());
    });

    std::cout << "Started server\n";
    test.start();
    return 0;
}
