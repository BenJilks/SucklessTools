#include "useless.hpp"
#include <unistd.h>
#include <iostream>
#include <termios.h>

UselessOutput::UselessOutput()
{
    struct termios config;
    if (tcgetattr(0, &config) < 0)
        perror("tcgetattr()");

    config.c_lflag &= ~ICANON;
    config.c_lflag &= ~ECHO;
    if (tcsetattr(0, TCSANOW, &config) < 0)
        perror("tcsetattr()");

    std::cout << "Started useless terminal\n";
}

void UselessOutput::write(std::string_view buf)
{
    std::cout << buf;
    std::cout.flush();
}

int UselessOutput::input_file() const
{
    return STDOUT_FILENO;
}
