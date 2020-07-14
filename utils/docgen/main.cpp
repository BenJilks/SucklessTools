#include <iostream>
#include <fstream>
#include "lexer.hpp"

int main()
{
    std::cout << "Hello, DocGen!!\n";
    
    std::ifstream in("../main.cpp");
    Lexer lexer(in);
    lexer.add_keyword("include");
    lexer.add_keyword("return");
    lexer.add_symbol("::"); 
    lexer.add_string_type('"');
    lexer.add_string_type('\'', true);

    for (;;)
    {
        auto token = lexer.next();
        if (!token)
            break;
            
        std::cout << (int)token->type << " - " << token->data << "\n";
    }
    
    return 0;
}
