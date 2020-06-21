#include "value.hpp"
#include "../entry.hpp"
#include "../row.hpp"
#include <iostream>
using namespace DB;
using namespace DB::Sql;

template<typename T>
static auto value_of(const Value &value)
{
    return static_cast<const T&>(value).data();
}

template<typename EntryType, typename ValueType>
static auto entry_from_literal(const Value &value)
{
    return std::make_unique<EntryType>(value_of<ValueType>(value));
}

template<typename ValueType, typename EntryType>
static auto literal_from_entry(const Entry &value)
{
    return std::make_unique<ValueType>(static_cast<const EntryType&>(value).data());
}

std::unique_ptr<Entry> Value::as_entry() const
{
    switch (m_type)
    {
        case Integer: return entry_from_literal<IntegerEntry, ValueInteger>(*this);
        case String: return entry_from_literal<CharEntry, ValueString>(*this);
        default:
            assert (false);
    }
}

std::unique_ptr<Value> ValueColumn::evaluate(const Row &row) const
{
    const auto &entry = row[data()];

    switch (entry->data_type().primitive())
    {
        case DataType::Integer:
            return literal_from_entry<ValueInteger, IntegerEntry>(*entry);
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
        case Equals: return make_result(left == right);
        default:
            assert (false);
    }
}

std::unique_ptr<Value> ValueCondition::evaluate(const Row &row) const
{
    if (m_left->type() == Value::Column || m_right->type() == Value::Column)
    {
        auto sub_condition = ValueCondition(
            m_left->evaluate(row), m_operation, m_right->evaluate(row));

        return sub_condition.evaluate(row);
    }

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
            break;
        default:
            assert (false);
    }
}
