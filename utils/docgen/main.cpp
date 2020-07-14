#include <iostream>
#include <fstream>
#include "lexer.hpp"

int main()
{
    std::cout << "Hello, DocGen!!\n";
    
    std::ifstream in("../main.cpp");
    Lexer lexer(in);
    lexer.load("../langs/cpp.json");

    for (;;)
    {
        auto token = lexer.next();
        if (!token)
            break;
            
        std::cout << (int)token->type << " - " << token->data << "\n";
    }
    
    return 0;
}
