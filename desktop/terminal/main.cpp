#include "terminal.hpp"
#include "output/xlib.hpp"
#include <libprofile/profile.hpp>

int main()
{
    XLibOutput out;
    Terminal terminal(std::move(out));

    Profile::Profiler::the().print_results();
}
