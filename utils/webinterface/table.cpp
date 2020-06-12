#include "table.hpp"
#include <algorithm>
using namespace Web;

std::optional<Table> Table::make_impl(std::vector<std::string> &&args)
{
    auto headers = [&args]()
    {
        std::string out;
        for (const auto &arg : args)
            out += "<th>" + arg + "</th>";
        return out;
    };

    auto data = [&args]()
    {
        std::string out;
        for (const auto &arg : args)
            out += "<td>[[ " + arg + " ]]</td>";
        return out;
    };

    const auto table_html =
        "<table>\n"
        "\t<thead>\n"
        "\t\t" + headers() + "\n"
        "\t</thead>\n"
        "[[ rows ]]\n"
        "</table>";
    const auto row_html = "\t<tr>" + data() + "</tr>\n";

    auto table = Template::load_from_memory(std::move(table_html));
    auto row = Template::load_from_memory(std::move(row_html));
    if (!table || !row)
        return std::nullopt;

    return Table(std::move(*table), std::move(*row));
}

Template &Table::add_row()
{
    m_rows.emplace_back(m_row);
    return m_rows.back();
}

void Table::sort_by(const std::string &column, SortMode sort_mode)
{
    std::sort(m_rows.begin(), m_rows.end(), [&column, sort_mode](auto &a, auto &b)
    {
        switch (sort_mode)
        {
            case SortMode::Ascending: return a[column] < b[column];
            case SortMode::Descending: return a[column] > b[column];
        }
    });
}

std::string Table::build()
{
    std::string rows;
    for (const auto &row : m_rows)
        rows += row.build();

    m_table["rows"] = rows;
    return m_table.build();
}
