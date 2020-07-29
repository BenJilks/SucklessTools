#pragma once
#include "output/output.hpp"
#include "cursor.hpp"
#include <vector>

class Terminal
{
public:
    Terminal(Output &&output);
    
private:
    void init();
    void run_event_loop();

    Output &m_output;
    int m_master;
    int m_slave_pid;

};
