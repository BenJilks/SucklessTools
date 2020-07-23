#pragma once

#include <QWidget>
#include <QLabel>
#include "email.hpp"

class EmailViewer : QWidget
{
    Q_OBJECT

public:
    EmailViewer(QWidget *parent = nullptr);

    void set_email(const Email&);

private:
    QLabel *m_view;
    const Email *m_email { nullptr };

};
