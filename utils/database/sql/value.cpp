#include "value.hpp"
#include "../entry.hpp"
using namespace DB;
using namespace DB::Sql;

template<typename T>
static auto get_literal_value(const Value &value)
{
    return static_cast<const T&>(value).data();
}

template<typename EntryType, typename ValueType>
static auto make_literal(const Value &value)
{
    return std::make_unique<EntryType>(get_literal_value<ValueType>(value));
}

std::unique_ptr<Entry> Value::as_entry() const
{
    switch (m_type)
    {
        case Integer: return make_literal<IntegerEntry, ValueInteger>(*this);
        default:
            assert (false);
    }
}

ValueCondition::ValueCondition(std::unique_ptr<Value> left, Operation operation, std::unique_ptr<Value> right)
    : Value(Condition)
    , m_left(std::move(left))
    , m_right(std::move(right))
    , m_operation(operation) {}

template<typename Left, typename Right>
std::unique_ptr<Value> ValueCondition::operation() const
{
    auto left = static_cast<Left&>(*m_left).data();
    auto right = static_cast<Right&>(*m_right).data();
    auto make_result = [&](bool result)
    {
        return std::make_unique<ValueBoolean>(result);
    };

    switch(m_operation)
    {
        case MoreThan: return make_result(left > right);
        default:
            assert (false);
    }
}

std::unique_ptr<Value> ValueCondition::evaluate() const
{
    switch(m_left->type())
    {
        case Value::Integer:
            switch (m_right->type())
            {
                case Value::Integer:
                    return operation<ValueInteger, ValueInteger>();
                default:
                    assert (false);
            }
        default:
            assert (false);
    }
}
