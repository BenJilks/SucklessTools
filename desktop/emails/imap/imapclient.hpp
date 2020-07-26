#pragma once
#include <curl/curl.h>
#include <vector>
#include <string>
#include "email.hpp"

namespace Imap
{

    class Client
    {
    public:
        explicit Client();
        ~Client();

        std::vector<std::string> fetch_mailboxes();
        std::vector<int> fetch_mailbox_uids(const std::string &mailbox, int count = -1);
        std::unique_ptr<Email> fetch_email(const std::string &mailbox, int uid);

    private:
        struct Memory
        {
            std::string result;
        };

        CURL *m_curl;
        Memory m_memory;

        static size_t write_callback(void *buffer, size_t, size_t nmemb, void *data);
        bool execute();

    };

}
