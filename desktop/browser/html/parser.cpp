#include "parser.hpp"
#include <cassert>
using namespace HTML;

Parser::Parser(std::istream &in) 
    : m_lexer(in)
{
}

std::unique_ptr<Document> Parser::run()
{
    enum class State
    {
        Default,
        Html,
        Head,
        Body,
    };
    
    auto document = std::make_unique<Document>();
    auto state = State::Default;
    NodeStack node_stack;
    for (;;)
    {
        auto token_or_error = m_lexer.next();
        if (!token_or_error)
            break;
        
        auto &token = *token_or_error;
        switch (state)
        {
            case State::Default:
                if (token.type == Lexer::Text)
                    break;
                
                if (token.type == Lexer::StartTag)
                {
                    if (token.name == "html")
                    {
                        state = State::Html;
                        break;
                    }
                    
                    assert (false);
                }
                
                assert (false);
                break;
            
            case State::Html:
                if (token.type == Lexer::Text)
                    break;

                if (token.type == Lexer::StartTag)
                {
                    if (token.name == "head")
                    {
                        node_stack.push(std::make_unique<Head>());
                        state = State::Head;
                        break;
                    }
                    
                    if (token.name == "body")
                    {
                        node_stack.push(std::make_unique<Body>());
                        state = State::Body;
                        break;
                    }
                    
                    assert (false);
                }
                
                if (token.type == Lexer::EndTag)
                {
                    if (token.name == "html")
                    {
                        state = State::Default;
                        break;
                    }
                    
                    assert (false);
                }
                
                assert (false);
                break;
            
            case State::Head:
                if (token.type == Lexer::Text)
                    break;

                if (token.type == Lexer::EndTag)
                {
                    if (token.name == "head")
                    {
                        document->set_head(node_stack.pop());
                        state = State::Html;
                        break;
                    }
                    
                    assert (false);
                }
                
                assert (false);
                break;
            
            case State::Body:
                if (token.type == Lexer::Text)
                    break;

                if (token.type == Lexer::EndTag)
                {
                    if (token.name == "body")
                    {
                        document->set_body(node_stack.pop());
                        state = State::Html;
                        break;
                    }
                    
                    assert (false);
                }
                
                assert (false);
                break;
        }
    }
    
    return document;
}
