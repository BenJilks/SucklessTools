#pragma once
#include <curl/curl.h>

class ImapClient
{
public:
    ImapClient();

private:
    CURL *m_curl;

};
