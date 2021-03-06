#pragma once
#include "../forward.hpp"
#include <cassert>
#include <memory>
#include <type_traits>
#include <string>

namespace DB::Sql
{

    class Value
    {
    public:
        enum Type
        {
            Null,
            Integer,
            Float,
            Boolean,
            String,
        };
        
        Value()
            : m_type(Null) {}
        
        explicit Value(int64_t i)
            : m_type(Integer)
            , m_int(i) {}

        explicit Value(float f)
            : m_type(Float)
            , m_float(f) {}

        explicit Value(bool b)
            : m_type(Boolean)
            , m_bool(b) {}
        
        explicit Value(const std::string &str)
            : m_type(String)
            , m_str(str) {}

        inline Type type() const { return m_type; }
        inline int64_t as_int() const { assert(m_type == Integer); return m_int; }
        inline float as_float() const { assert(m_type == Float); return m_float; }
        inline bool as_bool() const { assert(m_type == Boolean); return m_bool; }
        inline const std::string &as_string() const { assert(m_type == String); return m_str; }
        
        std::unique_ptr<Entry> as_entry() const;
        
    private:
        Type m_type;
        
        int64_t m_int;
        float m_float;
        bool m_bool;
        std::string m_str;

    };
    
    class ValueNode
    {
    public:
        enum class Type
        {
            Value,
            Column,
            MoreThan,
            Equals,
            And,
        };
        
        explicit ValueNode(Value value)
            : m_type(Type::Value)
            , m_value(value) {}
        
        // Binary operator
        explicit ValueNode(std::unique_ptr<ValueNode> left, Type operation, std::unique_ptr<ValueNode> right)
            : m_type(operation)
            , m_left(std::move(left))
            , m_right(std::move(right)) {}

        // Unary operator
        explicit ValueNode(Type operation, std::unique_ptr<ValueNode> operand)
            : m_type(operation)
            , m_left(std::move(operand)) {}
        
        Value evaluate(const Row &row);
        
    private:
        Type m_type;
        Value m_value;
        std::unique_ptr<ValueNode> m_left { nullptr };
        std::unique_ptr<ValueNode> m_right { nullptr };
        
    };
    
}
