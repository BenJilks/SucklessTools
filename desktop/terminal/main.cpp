#include "terminal.hpp"
#include "output/xlib.hpp"

int main()
{
    XLibOutput out;
    Terminal terminal(std::move(out));
}
