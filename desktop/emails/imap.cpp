#include "imap.hpp"
#include <iostream>
#include <string>

static size_t my_fwrite(void *buffer, size_t, size_t nmemb, void*)
{
    std::cout << std::string_view((char*)buffer, nmemb) << "\n";
    return nmemb;
}

ImapClient::ImapClient()
{
    m_curl = curl_easy_init();
    if (!m_curl)
        return;

    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, my_fwrite);
    curl_easy_setopt(m_curl, CURLOPT_USERNAME, "benjyjilks@gmail.com");
    curl_easy_setopt(m_curl, CURLOPT_PASSWORD, "bjilkspeare");
    curl_easy_setopt(m_curl, CURLOPT_URL,
        "imaps://imap.gmail.com/INBOX;UID=1641");

//    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "FETCH 1 BODY");
    auto res = curl_easy_perform(m_curl);
    if (res != CURLE_OK)
        std::cout << "Error: " << curl_easy_strerror(res) << "\n";

    curl_easy_cleanup(m_curl);
}
