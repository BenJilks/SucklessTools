#include "value.hpp"
#include "../entry.hpp"
#include "../row.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
using namespace DB;
using namespace DB::Sql;

std::unique_ptr<Entry> Value::as_entry() const
{
    switch (m_type)
    {
        case Integer: return std::make_unique<BigIntEntry>(m_int);
        case Float: return std::make_unique<FloatEntry>(m_float);
        case String: return std::make_unique<CharEntry>(m_str);
        default:
            assert (false);
    }
}

static Value get_entry_value(const std::unique_ptr<Entry> &entry)
{
    switch (entry->data_type().primitive())
    {
        case DataType::Integer: return Value((int64_t)entry->as_int());
        case DataType::BigInt: return Value(entry->as_long());
        case DataType::Float: return Value(entry->as_float());
        case DataType::Char: return Value(entry->as_string());
        case DataType::Text: return Value(entry->as_string());
        default:
            assert (false);
    }
}

template <typename Callback>
static Value operation(const Value &lhs, const Value &rhs, Callback callback)
{
    switch (lhs.type())
    {
        case Value::Type::Integer:
            switch (rhs.type())
            {
                case Value::Type::Integer:
                    return Value(callback(lhs.as_int(), rhs.as_int()));
                case Value::Type::Float:
                    return Value(callback(lhs.as_int(), rhs.as_float()));
                default:
                    assert (false);
            }
            break;
        case Value::Type::Float:
            switch (rhs.type())
            {
                case Value::Type::Integer:
                    return Value(callback(lhs.as_float(), rhs.as_int()));
                case Value::Type::Float:
                    return Value(callback(lhs.as_float(), rhs.as_float()));
                default:
                    assert (false);
            }
            break;
        case Value::Type::String:
            switch (rhs.type())
            {
                case Value::Type::String:
                    return Value(callback(lhs.as_string(), rhs.as_string()));
                default:
                    assert (false);
            }
        default:
            assert (false);
            break;
    }
}

Value ValueNode::evaluate(const Row &row)
{
    switch (m_type)
    {
        case Type::Value:
            assert (!m_left);
            assert (!m_right);
            return m_value;
        
        case Type::Column:
            assert (m_left);
            assert (!m_right);
            return get_entry_value(row[m_left->evaluate(row).as_string()]);
        
        case Type::MoreThan:
            assert (m_left);
            assert (m_right);
            return operation(
                m_left->evaluate(row), m_right->evaluate(row), 
                [&](auto a, auto b) { return a > b; });
        
        case Type::Equals:
            assert (m_left);
            assert (m_right);
            return operation(
                m_left->evaluate(row), m_right->evaluate(row), 
                [&](auto a, auto b) { return a == b; });

        case Type::And:
        {
            assert (m_left);
            assert (m_right);
            auto left = m_left->evaluate(row);
            if (left.type() != Value::Boolean || left.as_bool() == false)
                return Value(false);

            auto right = m_right->evaluate(row);
            if (right.type() != Value::Boolean || right.as_bool() == false)
                return Value(false);

            return Value(true);
        }

        default:
            assert (false);
    }
}
