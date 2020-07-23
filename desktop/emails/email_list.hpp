#pragma once

#include <QScrollArea>
#include <QLabel>
#include <functional>
#include "email.hpp"

class EmailList;
class EmailListItem : public QWidget
{
    Q_OBJECT

public:
    EmailListItem(const Email&, EmailList&);

    void deselect();

private:
    const Email &m_email;
    EmailList &m_list;

    QLabel *m_preview_text;
    bool m_is_selected { false };

    void resizeEvent(QResizeEvent*) override;
    void enterEvent(QEvent*) override;
    void leaveEvent(QEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

};

class EmailList : public QScrollArea
{
    Q_OBJECT

public:
    EmailList(QWidget *parent = nullptr);

    void add(const Email&);
    void deselect_all();

    std::function<void(const Email&)> on_select;

private:
    QLayout *m_layout;
    std::vector<EmailListItem*> m_email_items;

};
