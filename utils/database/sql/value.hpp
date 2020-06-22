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
        virtual ~Value() {}

        enum Type
        {
            Integer,
            Boolean,
            String,
            Column,
            Condition,
        };

        inline Type type() const { return m_type; }

        virtual std::unique_ptr<Value> evaluate(const Row&) const = 0;
        std::unique_ptr<Entry> as_entry() const;

    protected:
        explicit Value(Type type)
            : m_type(type) {}

    private:
        Type m_type;

    };

    template<typename T, Value::Type literal_type>
    class ValueLiteral : public Value
    {
    public:
        explicit ValueLiteral(T data)
            : Value(literal_type)
            , m_data(data)
        {}

        virtual std::unique_ptr<Value> evaluate(const DB::Row&) const override
        {
            return std::make_unique<ValueLiteral<T, literal_type>>(m_data);
        }

        inline T data() const { return m_data; }

    private:
        T m_data;

    };

    typedef ValueLiteral<int, Value::Integer> ValueInteger;
    typedef ValueLiteral<bool, Value::Boolean> ValueBoolean;
    class ValueString : public ValueLiteral<std::string, Value::String>
    {
    public:
        ValueString(const std::string &str)
            : ValueLiteral(str) {}

        ValueString(std::string_view str)
            : ValueLiteral(std::string(str)) {}
    };

    class ValueColumn : public ValueLiteral<std::string, Value::Column>
    {
    public:
        explicit ValueColumn(const std::string &data)
            : ValueLiteral(data) {}

        virtual std::unique_ptr<Value> evaluate(const DB::Row&) const override;

    private:
    };

    class ValueCondition : public Value
    {
    public:
        enum Operation
        {
            MoreThan,
            Equals,
        };

        ValueCondition(std::unique_ptr<Value> left, Operation, std::unique_ptr<Value> right);

        virtual std::unique_ptr<Value> evaluate(const DB::Row&) const override;

    private:
        template<typename Left, typename Right>
        std::unique_ptr<Value> operation() const;

        std::unique_ptr<Value> m_left;
        std::unique_ptr<Value> m_right;
        Operation m_operation;

    };

}
