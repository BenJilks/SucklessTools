#pragma once
#include "lexer.hpp"
#include "Document.hpp"

namespace HTML
{
    
    class Parser
    {
    public:
        Parser(std::istream&);
        
        std::unique_ptr<Document> run();
        
    private:
        class NodeStack
        {
        public:
            NodeStack() {}
            
            void push(std::unique_ptr<DOMNode> node) { m_impl.push_back(std::move(node)); }
            std::unique_ptr<DOMNode> pop()
            {
                auto node = std::move(m_impl.back());
                m_impl.pop_back();
                return node;
            }
            
        private:
            std::vector<std::unique_ptr<DOMNode>> m_impl;
        
        };
        
        Lexer m_lexer;
    
    };
    
}
