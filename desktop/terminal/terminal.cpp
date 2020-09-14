#include "terminal.hpp"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <string>
#include <iostream>
#include <libprofile/profile.hpp>

#if __FreeBSD__
#include <libutil.h>
#else
#include <pty.h>
#endif

static bool wait_flag = false;

Terminal::Terminal(Output &&output) 
    : m_output(output)
{
    output.on_resize = [&](auto size)
    {
        if (ioctl(m_master, TIOCSWINSZ, &size) < 0)
        {
            perror("ioctl(TIOCSWINSZ)");
            return;
        }
    };
    output.init();
    
    init();
    run_event_loop();
}

void Terminal::init()
{
    Profile::Timer timer("Terminal::init");
    wait_flag = true;

    int slave;
    if (openpty(&m_master, &slave, NULL, NULL, NULL) < 0)
    {
        perror("openpty()");
        return;
    }
    
    m_slave_pid = fork();
    if (m_slave_pid == -1)
    {
        perror("fork()");
        return;
    }
    
    if (m_slave_pid == 0)
    {
        setsid(); // Create a new process group
        dup2(slave, 0);
        dup2(slave, 1);
        dup2(slave, 2); 
        if (ioctl(slave, TIOCSCTTY, nullptr) < 0)
        {
            perror("ioctl(TIOCSCTTY)");
            exit(-1);
        }
        close(slave);
        close(m_master);
    
        const auto *shell = "/usr/bin/bash"; //getenv("SHELL");
        setenv("TERM", "xterm-16color", 1);
        if (execl(shell, shell, nullptr) < 0)
            perror("system()");
        exit(-1);
    }
    
    close(slave);
    ioctl(m_master, TIOCPKT, 1);
}

void Terminal::run_event_loop()
{
    Profile::Timer timer("Terminal::run_event_loop");
    fd_set fds;
    
    while (!m_output.should_close())
    {
        FD_ZERO(&fds);
        FD_SET(m_master, &fds);
        FD_SET(m_output.input_file(), &fds);
        if (select((m_master + m_output.input_file()) + 1, &fds, NULL, NULL, NULL) < 0)
        {
            perror("select(master)");
            break;
        }
        
        if (FD_ISSET(m_master, &fds))
        {
            char read_buf[1024];
            auto ret = read(m_master, read_buf, sizeof(read_buf));
            if (ret < 0)
            {
                perror("read()");
                break;
            }
            
            m_output.out(std::string_view(read_buf, ret));

            // Exit when slave pid has exited
            int status;
            if (waitpid(m_slave_pid, &status, WNOHANG) != 0)
                break;
        }
        
        if (FD_ISSET(m_output.input_file(), &fds))
        {
            auto input = m_output.update();
            if (!input.empty())
            {
                if (write(m_master, input.data(), input.length()) < 0)
                {
                    perror("write()");
                    break;
                }
            }
        }
    }
    
    std::cout << "Terminal exited\n";
    std::cout.flush();
}
