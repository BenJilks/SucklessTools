#include "window.hpp"
#include "imap.hpp"
#include <curl/curl.h>
#include <QApplication>

int main()
{
    ImapClient client;
    return 0;
}

int main2(int argc, char *argv[])
{
    QApplication app(argc, argv);
    auto *curl = curl_easy_init();
    if (curl)
    {
        curl_easy_cleanup(curl);
    }

    Window window;
    window.show();
    return app.exec();
}
