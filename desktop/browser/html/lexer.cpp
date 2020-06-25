#include "lexer.hpp"
#include <cassert>
#include <iostream>
#include <sstream>
using namespace HTML;

Lexer::Lexer(std::istream &in)
    : m_in(in)
{
}

std::optional<Lexer::Token> Lexer::next()
{
    for (;;)
    {
        if (!m_should_reconsume)
            m_current_char = m_in.get();
        
        if (m_current_char == -1)
            return std::nullopt;
        
        m_should_reconsume = false;
        switch (m_state)
        {
            case State::Default:
                if (m_current_char == '<')
                {
                    m_current_token = {};
                    if (!m_buffer.empty())
                    {
                        m_current_token.type = Text;
                        m_current_token.name = m_buffer;

                        m_buffer.clear();
                        m_should_reconsume = true;
                        return m_current_token;
                    }
                    
                    m_current_token.type = StartTag;
                    m_state = State::TagStart;
                    break;
                }
                
                m_buffer += m_current_char;
                break;
            
            case State::Name:
                if (isalnum(m_current_char))
                {
                    m_buffer += m_current_char;
                    break;
                }
                
                m_should_reconsume = true;
                m_state = m_return_state;
                break;
            
            case State::TagStart:
                if (isspace(m_current_char))
                    break;
                
                if (m_current_char == '/')
                {
                    m_current_token.type = EndTag;
                    break;
                }
                
                if (isalnum(m_current_char))
                {
                    m_should_reconsume = true;
                    m_state = State::Name;
                    m_return_state = State::TagName;
                    break;
                }
                
                assert (false);
            
            case State::TagName:
                m_current_token.name = m_buffer;
                m_buffer.clear();
                
                m_should_reconsume = true;
                m_state = State::TagAttr;
                break;
            
            case State::TagAttr:
                if (isspace(m_current_char))
                    break;
                
                if (isalnum(m_current_char))
                {
                    m_should_reconsume = true;
                    m_state = State::Name;
                    m_return_state = State::TagAttrName;
                    break;
                }
                
                if (m_current_char == '>')
                {
                    m_state = State::Default;
                    return m_current_token;
                }
            
            case State::TagAttrName:
                m_current_token.attrs.push_back({ m_buffer, "" });
                m_buffer.clear();
                
                m_should_reconsume = true;
                m_state = State::TagAttrEquals;
                break;
                
            case State::TagAttrEquals:
                if (isspace(m_current_char))
                    break;
                
                if (m_current_char == '=')
                {
                    m_state = State::TagAttrValueStart;
                    break;
                }
                
                assert (false);
                break;
            
            case State::TagAttrValueStart:
                if (isspace(m_current_char))
                    break;
                
                if (m_current_char == '"')
                {
                    m_state = State::TagAttrValue;
                    break;
                }
                
                assert (false);
                break;
            
            case State::TagAttrValue:
                if (m_current_char == '"')
                {
                    m_current_token.attrs.back().second = m_buffer;
                    m_buffer.clear();
                    m_state = State::TagAttr;
                    break;
                }
                
                m_buffer += m_current_char;
                break;
        }
    }
}
