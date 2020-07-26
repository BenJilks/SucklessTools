#include "imapclient.hpp"
#include "imapparser.hpp"
#include "imapcontent.hpp"
#include <iostream>
#include <string>
#include <stdlib.h>
using namespace Imap;

size_t Client::write_callback(void *buffer, size_t, size_t nmemb, void *data)
{
    auto &memory = *static_cast<Memory*>(data);
    memory.result += std::string((char*)buffer, nmemb);

    return nmemb;
}

Client::Client()
{
    m_curl = curl_easy_init();
    if (!m_curl)
        return;

    curl_easy_setopt(m_curl, CURLOPT_PROTOCOLS, CURLPROTO_IMAPS);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_memory);
    curl_easy_setopt(m_curl, CURLOPT_USERNAME, "benjyjilks@gmail.com");
    curl_easy_setopt(m_curl, CURLOPT_PASSWORD, "bjilkspeare");
}

std::vector<std::string> Client::fetch_mailboxes()
{
    curl_easy_setopt(m_curl, CURLOPT_URL, "imaps://imap.gmail.com");
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "LIST \"*\" \"*\"");

    if (!execute())
        return {};

    std::vector<std::string> out;
    Parser parser(m_memory.result);
    while (!parser.is_eof())
    {
        parser.parse_type();
        parser.parse_flags();
        parser.parse_string();
        out.push_back(parser.parse_string());
        parser.next_line();
    }
    return out;
}

std::vector<int> Client::fetch_mailbox_uids(const std::string &mailbox, int count)
{
    auto count_str = (count == -1 ? "*" : std::to_string(count));

    curl_easy_setopt(m_curl, CURLOPT_URL, ("imaps://imap.gmail.com/" + mailbox).c_str());
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, ("FETCH 1:" + count_str + " (UID)").c_str());
    if (!execute())
        return {};

    std::vector<int> uids;
    Parser parser(m_memory.result);
    while (!parser.is_eof())
    {
        parser.parse_number();
        parser.parse_type();
        parser.parse_char('(');
        parser.parse_type();
        auto uid = parser.parse_number();
        parser.parse_char(')');

        uids.push_back(uid);
        parser.next_line();
    }
    return uids;
}

std::unique_ptr<Email> Client::fetch_email(const std::string &mailbox, int uid)
{
    curl_easy_setopt(m_curl, CURLOPT_URL, ("imaps://imap.gmail.com/"
        + mailbox + ";UID=" + std::to_string(uid)).c_str());
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, nullptr);

    auto make_error_email = []()
    {
        return std::make_unique<Email>("Error", "Error");
    };

    if (!execute())
        return make_error_email();

    Content content(m_memory.result);
    if (!content.parse())
        return make_error_email();

    return std::move(content.email());
}

bool Client::execute()
{
    m_memory.result.clear();
    auto res = curl_easy_perform(m_curl);
    if (res != CURLE_OK)
    {
        std::cout << "Error: " << curl_easy_strerror(res) << "\n";
        return false;
    }

    return true;
}

Client::~Client()
{
    if (m_curl)
        curl_easy_cleanup(m_curl);
}
