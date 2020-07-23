#include "email_list.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>
#include <iostream>

EmailListItem::EmailListItem(const Email &email, EmailList &list)
    : QWidget(&list)
    , m_email(email)
    , m_list(list)
{
    QPalette pallet;

    auto layout = new QVBoxLayout(this);
    setLayout(layout);
    setAttribute(Qt::WA_Hover);
    setAutoFillBackground(true);

    auto from_text = new QLabel(m_email.from(), this);
    from_text->setFont(QFont("Liberation Sans Regular", 19));
    layout->addWidget(from_text);

    auto subject_text = new QLabel(m_email.subject(), this);
    layout->addWidget(subject_text);

    m_preview_text = new QLabel(m_email.preview(), this);
    pallet.setColor(m_preview_text->foregroundRole(), Qt::gray);
    m_preview_text->setPalette(pallet);
    layout->addWidget(m_preview_text);
}

void EmailListItem::deselect()
{
    QPalette pallet;
    setPalette(pallet);

    m_is_selected = false;
}

void EmailListItem::resizeEvent(QResizeEvent *event)
{
    auto size = event->size();

    QFontMetrics metrix(m_preview_text->font());
    QString clipped_text = metrix.elidedText(m_email.preview(),
        Qt::ElideRight, size.width() - 20);
    m_preview_text->setText(clipped_text);
}

void EmailListItem::enterEvent(QEvent*)
{
    if (!m_is_selected)
    {
        QPalette pallet;
        pallet.setColor(backgroundRole(), pallet.light().color());
        setPalette(pallet);
    }
}

void EmailListItem::leaveEvent(QEvent*)
{
    if (!m_is_selected)
    {
        QPalette pallet;
        setPalette(pallet);
    }
}

void EmailListItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        m_list.deselect_all();
        m_is_selected = true;

        QPalette pallet;
        pallet.setColor(backgroundRole(), pallet.highlight().color());
        setPalette(pallet);

        if (m_list.on_select)
            m_list.on_select(m_email);
    }
}

EmailList::EmailList(QWidget *parent)
    : QScrollArea(parent)
{
    auto list = new QWidget(this);
    m_layout = new QVBoxLayout(list);
    list->setLayout(m_layout);
    list->setSizePolicy(QSizePolicy::Policy::Ignored, QSizePolicy::MinimumExpanding);
    setWidget(list);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
}

void EmailList::add(const Email &email)
{
    auto email_item = new EmailListItem(email, *this);
    m_layout->addWidget(email_item);
    m_email_items.push_back(email_item);
}

void EmailList::deselect_all()
{
    for (const auto &item : m_email_items)
        item->deselect();
}
