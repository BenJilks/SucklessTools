#pragma once

namespace HTML
{
    
    class DOMNode
    {
    public:
        enum Type
        {
            Document,
            Element,
        };
        
        virtual void dump(std::ostream&, int indent = 0) = 0;
        
    protected:
        DOMNode(Type type)
            : m_type(type) {}
            
        std::string dump_indent(int indent)
        {
            std::string out;
            for (int i = 0; i < indent; i++)
                out += "\t";
            return out;
        }

    private:
        DOMNode *parent;
        Type m_type;
        
    };
    
}
