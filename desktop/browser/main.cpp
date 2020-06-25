#include "html/parser.hpp"
#include <iostream>
#include <sstream>

int main()
{
    std::cout << "Hello, Web!!!\n";
    
    auto test_html = 
        "<html>"
        "   <head>"
        "   </head>"
        "   <body>"
        "   </body>"
        "</html>";
        
    std::istringstream stream(test_html);
    auto parser = HTML::Parser(stream);
    auto doc = parser.run();
    doc->dump(std::cout);
    
    return 0;
}
 
