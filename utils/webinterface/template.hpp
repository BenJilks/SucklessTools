#pragma once
#include <optional>
#include <string>
#include <map>
#include <vector>

namespace Web
{

    class Template
    {
    public:
        static std::optional<Template> load_from_file(const std::string &path);
        static std::optional<Template> load_from_memory(const std::string &&data);

        void set(const std::string &name, const std::string &value);
        std::string build() const;
        std::string &operator[](const std::string &name);

    private:
        Template(const std::string &&data)
            : m_data(std::move(data)) {}

        struct Label
        {
            std::string name;
            std::string value;
            std::vector<size_t> positions;
        };

        Label make_label(const std::string &name);

        std::string m_data;
        std::map<std::string, Label> m_labels;
    };

}
