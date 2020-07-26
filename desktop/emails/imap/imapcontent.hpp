#pragma once
#include <string>
#include "../email.hpp"

namespace Imap
{

    class Content
    {
    public:
        Content(const std::string &in)
            : m_in(in) {}

        bool parse();

        const std::unique_ptr<Email> &email() const { return m_email; }
        std::unique_ptr<Email> &email() { return m_email; }

    private:
        const std::string &m_in;
        std::unique_ptr<Email> m_email { nullptr };

    };

}
