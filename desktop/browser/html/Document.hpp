#pragma once
#include "domnode.hpp"
#include <vector>
#include <memory>

namespace HTML
{
    
    class Element : public DOMNode
    {
    public:
        enum ElementType
        {
            Head,
            Body,
        };
        
        virtual void dump(std::ostream &out, int indent = 0) override
        {
            out << dump_indent(indent) << "Element:\n";
            for (const auto &child : m_children)
                child->dump(out, indent + 1);
        }
        
    protected:
        Element(ElementType type)
            : DOMNode(Type::Element)
            , m_element_type(type) {}

    private:
        std::vector<std::unique_ptr<DOMNode>> m_children;
        ElementType m_element_type;
        
    };
    
    class Head final : public Element
    {
    public:
        Head()
            : Element(ElementType::Head) {}
        
        virtual void dump(std::ostream &out, int indent = 0) override
        {
            out << dump_indent(indent) << "Head:\n";
            Element::dump(out, indent);
        }

    };
    
    class Body final : public Element
    {
    public:
        Body()
            : Element(ElementType::Body) {}
        
        virtual void dump(std::ostream &out, int indent = 0) override
        {
            out << dump_indent(indent) << "Body:\n";
            Element::dump(out, indent);
        }
    };
    
    class Document final : public DOMNode
    {
    public:
        Document()
            : DOMNode(Type::Document) {}
        
        inline const Head *head() const { return static_cast<const Head*>(m_head.get()); }
        inline const Body *body() const { return static_cast<const Body*>(m_body.get()); }
        
        void set_head(std::unique_ptr<DOMNode> head) { m_head = std::move(head); }
        void set_body(std::unique_ptr<DOMNode> body) { m_body = std::move(body); }
        
        virtual void dump(std::ostream &out, int indent = 0) override
        {
            out << dump_indent(indent) << "Document:\n";
            if (m_head) 
                m_head->dump(out, indent + 1);
            if (m_body) 
                m_body->dump(out, indent + 1);
        }

    private:
        std::unique_ptr<DOMNode> m_head;
        std::unique_ptr<DOMNode> m_body;
    
    };
    
}
