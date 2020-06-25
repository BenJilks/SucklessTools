#pragma once
#include <vector>
#include <istream>
#include <optional>

namespace HTML
{
    
    class Lexer
    {
    public:
        enum Type
        {
            Text,
            StartTag,
            EndTag,
        };
        
        struct Token
        {
            std::string name;
            std::vector<std::pair<std::string, std::string>> attrs;
            Type type;
        };
        
        Lexer(std::istream&);
        
        std::optional<Token> next();
        
    private:
        enum class State
        {
            Default,
            Name,
            TagStart,
            TagName,
            TagAttr,
            TagAttrName,
            TagAttrEquals,
            TagAttrValueStart,
            TagAttrValue,
        };
        
        std::istream &m_in;
        
        State m_state { State::Default };
        State m_return_state { State::Default };
        char m_current_char;
        bool m_should_reconsume { false };
        
        Token m_current_token;
        std::string m_buffer;
        
    };
    
}
