#pragma once
#include "output.hpp"

class UselessOutput final : public Output
{
public:
    UselessOutput();
    
    virtual void write(std::string_view buf) override;
    virtual int input_file() const override;
    
private:
    

};
