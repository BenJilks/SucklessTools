#include "emails.h"
#include <QSplitter>
#include <QPushButton>

EMails::EMails(QWidget *parent)
    : QMainWindow(parent)
{
    auto splitter = new QSplitter(Qt::Orientation::Horizontal, this);
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCentralWidget(splitter);

    auto left = new QPushButton("left", splitter);
    left->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto right = new QPushButton("right", splitter);
    right->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
}

EMails::~EMails()
{
}
