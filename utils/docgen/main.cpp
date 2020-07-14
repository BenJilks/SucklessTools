#include <iostream>
#include <fstream>
#include "lexer.hpp"
#include "structure/codeunit.hpp"

int main()
{
    std::cout << "Hello, DocGen!!\n";
    
    std::ifstream in("../main.cpp");
    Lexer lexer(in);
    lexer.load("../syntax/cpp.json");

    for (;;)
    {
        auto token = lexer.next();
        if (!token)
            break;
            
        std::cout << (int)token->type << " - " << token->data << "\n";
    }
    
    return 0;
}
