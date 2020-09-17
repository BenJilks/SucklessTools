#pragma once

#include <string>
#include <vector>

class Category
{
public:
    Category(const std::string &name)
        : m_name(name) {}

private:
    std::string m_name;

};

class Item
{
public:
    Item(const std::string &name, const Category *category = nullptr)
        : m_name(name)
        , m_category(category) {}

    const std::string &name() const { return m_name; }
    const Category *category() const { return m_category; }
    bool has_category() const { return m_category != nullptr; }

private:
    std::string m_name;
    const Category *m_category;

};

class ItemList
{
public:
    ItemList() = default;

    void add(const Item &item)
    {
        m_items.push_back(item);
    }
    int size() const { return m_items.size(); }

private:
    std::vector<Item> m_items;

};
