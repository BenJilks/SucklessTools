#pragma once

#include <QMainWindow>
#include "email_viewer.hpp"

class Window : public QMainWindow
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);
    ~Window();

private:
    EmailViewer *m_viewer;

};
