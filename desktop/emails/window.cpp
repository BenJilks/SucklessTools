#include "window.hpp"
#include "email_list.hpp"
#include "email_viewer.hpp"
#include <QSplitter>
#include <QPushButton>

Window::Window(QWidget *parent)
    : QMainWindow(parent)
{
    auto splitter = new QSplitter(Qt::Orientation::Horizontal, this);
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCentralWidget(splitter);

    auto left = new EmailList(splitter);
    m_viewer = new EmailViewer(splitter);

    left->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    for (int i = 0; i < 10; i++)
    {
        auto email = new Email("Bob", "Test");
        email->set_contents("This is a test email");
        left->add(*email);
    }

    left->on_select = [&](const Email &email)
    {
        m_viewer->set_email(email);
    };
}

Window::~Window()
{
}
