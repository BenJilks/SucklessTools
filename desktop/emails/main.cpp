#include "window.hpp"
#include "imap.hpp"
#include <QApplication>
#include <iostream>

int main()
{
    ImapClient client;
    for (auto uid : client.fetch_mailbox_uids("INBOX", 1))
        client.fetch_email("INBOX", uid);

    return 0;
}

int main2(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Window window;
    window.show();
    return app.exec();
}
