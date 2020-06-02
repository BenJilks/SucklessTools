#pragma once
#include "template.hpp"

namespace Web
{

    class Table
    {
    public:
        enum class SortMode
        {
            Ascending,
            Descending
        };

        template<typename ...Args>
        static std::optional<Table> make(Args ...args)
        {
            return make_impl({ args... });
        }

        Template &add_row();
        void sort_by(const std::string &column, SortMode = SortMode::Ascending);

        std::string build();

    private:
        Table(Template &&table, Template &&row)
            : m_table(std::move(table)), m_row(std::move(row)) {}

        static std::optional<Table> make_impl(std::vector<std::string> &&args);

        Template m_table;
        Template m_row;
        std::vector<Template> m_rows;
    };

}
