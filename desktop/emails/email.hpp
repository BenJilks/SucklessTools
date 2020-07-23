#pragma once

#include <QString>

class Email
{
public:
    Email(QString from, QString subject)
        : m_from(from)
        , m_subject(subject) {}

    inline void set_contents(QString contents) { m_contents = contents; }
    inline const QString &from() const { return m_from; }
    inline const QString &subject() const { return m_subject; }
    inline const QString &contents() const { return m_contents; }
    QString preview() const;

private:
    QString m_from;
    QString m_subject;
    QString m_contents;

};
