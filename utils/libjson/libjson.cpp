#include "libjson.hpp"
#include <iostream>
#include <cassert>
using namespace Json;

//#define DEBUG_PARSER

#define ENUMARATE_STATES                   \
    __ENUMARATE_STATE(Invalid)             \
    __ENUMARATE_STATE(Initial)             \
    __ENUMARATE_STATE(Value)               \
    __ENUMARATE_STATE(String)              \
    __ENUMARATE_STATE(StringEscape)        \
    __ENUMARATE_STATE(StringUnicode)       \
    __ENUMARATE_STATE(NumberStart)         \
    __ENUMARATE_STATE(Number)              \
    __ENUMARATE_STATE(NumberFraction)      \
    __ENUMARATE_STATE(NumberExponentStart) \
    __ENUMARATE_STATE(NumberExponent)      \
    __ENUMARATE_STATE(NumberDone)          \
    __ENUMARATE_STATE(ObjectStart)         \
    __ENUMARATE_STATE(ObjectKey)           \
    __ENUMARATE_STATE(ObjectSeportator)    \
    __ENUMARATE_STATE(ObjectAdd)           \
    __ENUMARATE_STATE(ArrayStart)          \
    __ENUMARATE_STATE(ArrayValue)          \
    __ENUMARATE_STATE(ArrayNext)           \
    __ENUMARATE_STATE(Done)

enum class State
{
#define __ENUMARATE_STATE(x) x,
    ENUMARATE_STATES
#undef __ENUMARATE_STATE
};

#ifdef DEBUG_PARSER

static const char* state_to_string(State state)
{
    switch (state)
    {
#define __ENUMARATE_STATE(x) \
    case State::x:           \
        return #x;
        ENUMARATE_STATES
#undef __ENUMARATE_STATE
    }

    return "Unkown";
}

#endif

template <typename... Args>
bool is_one_of(uint32_t rune, Args... args)
{
    auto options = { args... };
    for (auto option : options)
    {
        if (rune == option)
            return true;
    }

    return false;
}

Value Value::null() { return Value(Null); }
Value Value::object() { return Value(Object); }
Value Value::array() { return Value(Array); }
Value Value::string(const std::string& str) { return Value(str); }
Value Value::number(double n) { return Value(n); }
Value Value::boolean(bool b) { return Value(b); }
Value Value::parse(std::istream &&stream)
{
    if (!stream.good())
        return {};

    State state = State::Initial;
    std::vector<State> return_stack { State::Done };
    std::vector<Value> value_stack;
    std::vector<char> buffer(1024);
    size_t buffer_pointer = 0;

    // Pre allocate some memory to reduce allocations
    return_stack.reserve(20);
    value_stack.reserve(20);

    size_t line = 1;
    size_t column = 1;
    auto emit_error = [&line, &column](std::string_view message)
    {
        // TODO: Make this better
        std::cout << "[Error " << line << ":" << column << "] " << message << "\n";
    };

    bool should_reconsume = false;
    uint32_t rune = 0;
    for (;;)
    {
        if (!should_reconsume)
        {
            column += 1;
            rune = stream.get();

            if (rune == '\n')
            {
                line += 1;
                column = 1;
            }
        }
        should_reconsume = false;

#ifdef DEBUG_PARSER
        std::cout << "State: " << state_to_string(state) << "\n";
#endif

        if (state == State::Done)
            break;

        if (rune == -1)
        {
            emit_error("Unexpected end of file");
            break;
        }

        switch (state)
        {
        default:
            assert(false);
            break;

        case State::Initial:
            if (isspace(rune))
                break;

            if (is_one_of(rune, '{', '[', '"', 't', 'f') || isdigit(rune))
            {
                state = State::Value;
                should_reconsume = true;
                break;
            }

            // TODO: Report parser error
            assert(false);
            break;

        case State::Value:
            if (isspace(rune))
                break;

            if (rune == '{')
            {
                value_stack.push_back(Value::object());
                state = State::ObjectStart;
                break;
            }

            if (rune == '"')
            {
                state = State::String;
                break;
            }

            if (rune == '[')
            {
                value_stack.push_back(Value::array());
                state = State::ArrayStart;
                break;
            }

            if (isdigit(rune) || rune == '-')
            {
                should_reconsume = true;
                state = State::NumberStart;
                break;
            }

            if (value_stack.back().is_array() && rune == ']')
            {
                emit_error("Trainling ',' on end of array");
                return_stack.pop_back();
                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            if (rune == '}' && value_stack.size() >= 2
                && value_stack[value_stack.size() - 1].is_string()
                && value_stack[value_stack.size() - 2].is_object())
            {
                emit_error("Trainling ':' on end of object");
                value_stack.pop_back();
                return_stack.pop_back();
                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            emit_error("Invalid value");
            break;

        case State::String:
            if (rune == '"')
            {
                auto str = std::string(buffer.data(), buffer_pointer);
                value_stack.push_back(Value::string(std::move(str)));
                buffer_pointer = 0;

                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            if (rune == '\\')
            {
                state = State::StringEscape;
                break;
            }

            buffer[buffer_pointer++] = rune;
            break;

        case State::StringEscape:
            switch (rune)
            {
            case '"':
                buffer[buffer_pointer++] = '"';
                break;
            case '\\':
                buffer[buffer_pointer++] = '\\';
                break;
            case '/':
                buffer[buffer_pointer++] = '/';
                break;
            case 'b':
                buffer[buffer_pointer++] = '\b';
                break;
            case 'f':
                buffer[buffer_pointer++] = '\f';
                break;
            case 'n':
                buffer[buffer_pointer++] = '\n';
                break;
            case 'r':
                buffer[buffer_pointer++] = '\r';
                break;
            case 't':
                buffer[buffer_pointer++] = '\t';
                break;
            case 'u':
                break;
            default:
                assert(false);
                break;
            }

            if (rune == 'u')
            {
                state = State::StringUnicode;
                break;
            }

            state = State::String;
            break;

        case State::StringUnicode:
            // TODO: Unicode parsing
            assert(false);
            break;

        case State::NumberStart:
            // TODO: Should not accept any numbers starting with 0 other then 0, -0 and 0.x, ...
            if (rune == '-')
            {
                buffer[buffer_pointer++] = '-';
                state = State::Number;
                break;
            }

            should_reconsume = true;
            state = State::Number;
            break;

        case State::Number:
            if (isdigit(rune))
            {
                buffer[buffer_pointer++] = rune;
                break;
            }

            if (rune == '.')
            {
                buffer[buffer_pointer++] = rune;
                state = State::NumberFraction;
                break;
            }

            if (rune == 'E' || rune == 'e')
            {
                buffer[buffer_pointer++] = 'E';
                state = State::NumberExponentStart;
                break;
            }

            should_reconsume = true;
            state = State::NumberDone;
            break;

        case State::NumberFraction:
            if (isdigit(rune))
            {
                buffer[buffer_pointer++] = rune;
                break;
            }

            if (rune == 'E' || rune == 'e')
            {
                buffer[buffer_pointer++] = 'E';
                state = State::NumberExponentStart;
                break;
            }

            if (rune == '.')
            {
                emit_error("Multiple decmial places");
                break;
            }

            should_reconsume = true;
            state = State::NumberDone;
            break;

        case State::NumberExponentStart:
            if (rune == '-' || rune == '+')
            {
                buffer[buffer_pointer++] = rune;
                state = State::NumberExponent;
                break;
            }

            emit_error("Expected '+' or '-' before exponent");
            buffer[buffer_pointer++] = '+';
            state = State::NumberExponent;
            break;

        case State::NumberExponent:
            if (isdigit(rune))
            {
                buffer[buffer_pointer++] = rune;
                break;
            }

            should_reconsume = true;
            state = State::NumberDone;
            break;

        case State::NumberDone:
        {
            auto str = std::string_view(buffer.data(), buffer_pointer);
            buffer[buffer_pointer] = '\0';

            value_stack.push_back(Value::number(atof(str.data())));
            buffer_pointer = 0;

            should_reconsume = true;
            state = return_stack.back();
            return_stack.pop_back();
            break;
        }

        case State::ObjectStart:
            if (isspace(rune))
                break;

            if (rune == '}')
            {
                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            state = State::ObjectKey;
            should_reconsume = true;
            break;

        case State::ObjectKey:
            if (isspace(rune))
                break;

            if (rune == '"')
            {
                state = State::String;
                return_stack.push_back(State::ObjectSeportator);
                break;
            }

            if (rune == '}')
            {
                emit_error("Trailing ',' on end of object");
                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            emit_error("Expected object key");
            state = State::ObjectKey;
            break;

        case State::ObjectSeportator:
            if (isspace(rune))
                break;

            if (rune == ':')
            {
                return_stack.push_back(State::ObjectAdd);
                state = State::Value;
                break;
            }

            emit_error("Expected ':' object seporator");
            should_reconsume = true;
            state = State::ObjectKey;
            value_stack.pop_back();
            break;

        case State::ObjectAdd:
        {
            if (isspace(rune))
                break;

            auto value = std::move(value_stack.back());
            value_stack.pop_back();
            auto key = std::move(value_stack.back());
            value_stack.pop_back();

            auto &object = value_stack.back();
            assert (object.is_object());
            object[key.as_string()] = value;

            if (rune == ',')
            {
                state = State::ObjectKey;
                break;
            }

            if (rune == '}')
            {
                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            emit_error("Expected ',' on end of object key-value pair");
            should_reconsume = true;
            state = State::ObjectKey;
            break;
        }

        case State::ArrayStart:
            if (isspace(rune))
                break;

            if (rune == ']')
            {
                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            should_reconsume = true;
            state = State::ArrayValue;
            break;

        case State::ArrayValue:
            if (isspace(rune))
                break;

            should_reconsume = true;
            return_stack.push_back(State::ArrayNext);
            state = State::Value;
            break;

        case State::ArrayNext:
            if (isspace(rune))
                break;

            auto value = std::move(value_stack.back());
            value_stack.pop_back();

            auto& array = value_stack.back();
            assert (array.is_array());
            array.append(std::move(value));

            if (rune == ',')
            {
                state = State::ArrayValue;
                break;
            }

            if (rune == ']')
            {
                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            emit_error("Expected ',' between values in array");
            should_reconsume = true;
            state = State::ArrayValue;
            break;
        }
    }

    if (value_stack.size() != 1)
    {
        emit_error("There can only be one root value");
        return {};
    }

    return value_stack[0];
}

Value &Value::operator[] (const std::string &name)
{
    // TODO: Decide the best thing to do here
    static auto null = Value::null();
    if (m_type != Object)
        return null;

    return m_members[name];
}

const Value &Value::operator[] (const std::string &name) const
{
    // Delegate this to the non const version
    if (m_type != Object || m_members.find(name) == m_members.end())
        return (*const_cast<Value*>(this))[name];

    return m_members.at(name);
}

Value &Value::append(Value value)
{
    static auto null = Value::null();
    if (m_type != Array)
        return null;

    m_elements.push_back(value);
    return m_elements.back();
}

void Value::clear()
{
    switch(m_type)
    {
        case Object:
            m_members.clear();
            break;
        case Array:
            m_elements.clear();
            break;
        default:
            break;
    }
}

std::vector<std::pair<std::string, Value>> Value::as_key_value_pairs() const
{
    if (m_type != Object)
        return {};

    std::vector<std::pair<std::string, Value>> members;
    for (auto &it : m_members)
        members.push_back(it);
    return members;
}

std::vector<Value> Value::as_array() const
{
    if (m_type != Array)
        return {};

    return m_elements;
}

std::string Value::as_string(bool include_quotes) const
{
    switch (m_type)
    {
        case Null:
            return "null";
        case Object:
        {
            std::string out = "{ ";
            bool is_first = true;
            for (const auto &it : m_members)
            {
                if (!is_first)
                    out += ", ";
                is_first = false;

                out += "\"" + it.first + "\": " + it.second.as_string(true);
            }
            return out + " }";
        }
        case Array:
        {
            std::string out = "[ ";
            bool is_first = true;
            for (const auto &element : m_elements)
            {
                if (!is_first)
                    out += ", ";
                is_first = false;

                out += element.as_string(true);
            }
            return out + " }";
        }
        case String:
            if (include_quotes)
                return "\"" + m_string + "\"";
            return m_string;
        case Number:
            return std::to_string(m_number);
        case Boolean:
            return m_boolean ? "true" : "false";
        default:
            assert (false);
    }
}

std::string Value::pretty_print(int indent) const
{
    std::string out;
    auto print_indents = [&](int offset = 0)
    {
        for (int i = 0; i < indent + offset; i++)
            out += "\t";
    };
    print_indents();

    switch (m_type)
    {
        case Object:
        {
            out += "{\n";
            bool is_first = true;
            for (const auto &it : m_members)
            {
                if (!is_first)
                    out += ",\n";
                is_first = false;

                print_indents(1);
                out += "\"" + it.first + "\": ";

                if (it.second.is_object() || it.second.is_array())
                    out += "\n" + it.second.pretty_print(indent + 1);
                else
                    out += it.second.as_string(true);
            }

            out += "\n";
            print_indents();
            out += "}";
            break;
        }
        case Array:
        {
            out += "[\n";
            bool is_first = true;
            for (const auto &element : m_elements)
            {
                if (!is_first)
                    out += ",\n";
                is_first = false;
                out += element.pretty_print(indent + 1);
            }

            out += "\n";
            print_indents();
            out += "]";
            break;
        }
        default:
            out += as_string(true);
            break;
    }

    return out;
}

std::ostream &operator<< (std::ostream &stream, const Json::Value &value)
{
    stream << value.as_string();
    return stream;
}

double Value::as_number() const
{
    switch (m_type)
    {
        case Null:
            return 0;
        case Object:
            return 0;
        case Array:
            return 0;
        case String:
            return atoi(m_string.c_str());
        case Number:
            return m_number;
        case Boolean:
            return m_boolean ? 1 : 0;
        default:
            assert (false);
    }
}

bool Value::as_boolean() const
{
    switch (m_type)
    {
        case Null:
            return false;
        case Object:
            return m_members.size() > 0;
        case Array:
            return m_elements.size() > 0;
        case String:
            return !m_string.empty();
        case Number:
            return m_number == 0 ? false : true;
        case Boolean:
            return m_boolean;
        default:
            assert (false);
    }
}

Value &Value::otherwise(Value &&other)
{
    if (m_type == Null)
    {
        m_type = other.m_type;
        m_members = std::move(other.m_members);
        m_elements = std::move(other.m_elements);
        m_string = other.m_string;
        m_number = other.m_number;
        m_boolean = other.m_boolean;
    }

    return *this;
}
