#include "email.hpp"

QString Email::preview() const
{
    return m_contents.mid(0, 20);
}
