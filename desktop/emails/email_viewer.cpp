#include "email_viewer.hpp"
#include <QVBoxLayout>
#include <cassert>
#include <iostream>

EmailViewer::EmailViewer(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    setLayout(layout);

    m_view = new QLabel("N/A", this);
    layout->addWidget(m_view);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void EmailViewer::set_email(const Email &email)
{
    m_email = &email;
    m_view->setText(email.contents());
}
