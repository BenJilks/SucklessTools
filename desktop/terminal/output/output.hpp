#pragma once
#include <string>

class Output
{
public:
    Output() {}

    virtual void write(std::string_view buff) = 0;
    virtual int input_file() const = 0;
    virtual std::string update() { return ""; }
    
};
