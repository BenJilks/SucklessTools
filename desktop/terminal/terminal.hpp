#pragma once
#include "output/output.hpp"
#include "cursor.hpp"
#include <vector>

class Terminal
{
public:
    Terminal(Output &&output);
    ~Terminal();
    
private:
    void init();
    void run_event_loop();
    void on_terminal_update();
    void on_output_update();

    Output &m_output;
    int m_master;
    int m_slave_pid;
    char *m_buffer;

};
