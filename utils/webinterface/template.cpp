#include "template.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <tuple>
#include <algorithm>
using namespace Web;

std::optional<Template> Template::load_from_file(const std::string &path)
{
    std::ifstream file(path);
    if (!file.good())
        return std::nullopt;

    std::stringstream stream;
    stream << file.rdbuf();
    return Template(stream.str());
}

std::optional<Template> Template::load_from_memory(const std::string &&data)
{
    return Template(std::move(data));
}

Template::Label Template::make_label(const std::string &name)
{
    Label label;
    label.name = name;

    size_t start_pos = 0;
    while((start_pos = m_data.find(name, start_pos)) != std::string::npos)
        label.positions.push_back(start_pos++);
    return label;
}

void Template::set(const std::string &name, const std::string &value)
{
    const auto altered_name = "[[ " + name + " ]]";

    if (m_labels.find(altered_name) != m_labels.end())
    {
        auto &label = m_labels[altered_name];
        label.value = value;
        return;
    }

    auto label = make_label(altered_name);
    label.value = value;
    m_labels[altered_name] = label;
}

std::string &Template::operator[](const std::string &name)
{
    const auto altered_name = "[[ " + name + " ]]";
    if (m_labels.find(altered_name) != m_labels.end())
        return m_labels[altered_name].value;

    auto label = make_label(altered_name);
    m_labels[altered_name] = label;
    return m_labels[altered_name].value;
}

std::string Template::build() const
{
    // Create a sorted list of all labels by there permitions
    std::vector<std::pair<size_t, const Label*>> sorted_labels;
    for (const auto &it : m_labels)
    {
        for (size_t position : it.second.positions)
            sorted_labels.push_back(std::make_pair(position, &it.second));
    }
    std::sort(sorted_labels.begin(), sorted_labels.end(), [](auto &a, auto &b)
    {
        return a.first < b.first;
    });

    std::string out;
    size_t curr_src_index = 0;
    for (const auto &it : sorted_labels)
    {
        const auto position = it.first;
        const auto *label = it.second;

        out += m_data.substr(curr_src_index , position - curr_src_index);
        out += label->value;
        curr_src_index = position + label->name.length();
    }
    out += m_data.substr(curr_src_index, m_data.length() - curr_src_index);

    return out;
}
