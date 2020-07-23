#include "emails.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    EMails w;
    w.show();
    return a.exec();
}
