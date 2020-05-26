#include "libjson.hpp"
#include <assert.h>
#include <iostream>
#include <sstream>

//#define DEBUG_PARSER

using namespace Json;

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

Document Document::parse(std::istream&& stream)
{
    State state = State::Initial;
    std::vector<State> return_stack { State::Done };
    std::vector<std::shared_ptr<Value>> value_stack;
    std::string buffer;

    Document doc;
    size_t line = 1;
    size_t column = 1;
    auto emit_error = [&line, &column, &doc](std::string_view message) {
        Error error;
        error.line = line;
        error.column = column;
        error.message = message;
        doc.emit_error(std::move(error));
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
                value_stack.push_back(std::make_shared<Object>());
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
                value_stack.push_back(std::make_shared<Array>());
                state = State::ArrayStart;
                break;
            }

            if (isdigit(rune) || rune == '-')
            {
                should_reconsume = true;
                state = State::NumberStart;
                break;
            }

            if (value_stack.back()->is<Array>() && rune == ']')
            {
                emit_error("Trainling ',' on end of array");
                return_stack.pop_back();
                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            if (rune == '}' && value_stack.size() >= 2 && value_stack[value_stack.size() - 1]->is<String>() && value_stack[value_stack.size() - 2]->is<Object>())
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
                value_stack.push_back(std::make_shared<String>(buffer));
                buffer.clear();

                state = return_stack.back();
                return_stack.pop_back();
                break;
            }

            if (rune == '\\')
            {
                state = State::StringEscape;
                break;
            }

            buffer += rune;
            break;

        case State::StringEscape:
            switch (rune)
            {
            case '"':
                buffer += '"';
                break;
            case '\\':
                buffer += '\\';
                break;
            case '/':
                buffer += '/';
                break;
            case 'b':
                buffer += '\b';
                break;
            case 'f':
                buffer += '\f';
                break;
            case 'n':
                buffer += '\n';
                break;
            case 'r':
                buffer += '\r';
                break;
            case 't':
                buffer += '\t';
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
                buffer += "-";
                state = State::Number;
                break;
            }

            should_reconsume = true;
            state = State::Number;
            break;

        case State::Number:
            if (isdigit(rune))
            {
                buffer += rune;
                break;
            }

            if (rune == '.')
            {
                buffer += rune;
                state = State::NumberFraction;
                break;
            }

            if (rune == 'E' || rune == 'e')
            {
                buffer += 'E';
                state = State::NumberExponentStart;
                break;
            }

            should_reconsume = true;
            state = State::NumberDone;
            break;

        case State::NumberFraction:
            if (isdigit(rune))
            {
                buffer += rune;
                break;
            }

            if (rune == 'E' || rune == 'e')
            {
                buffer += 'E';
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
                buffer += rune;
                state = State::NumberExponent;
                break;
            }

            emit_error("Expected '+' or '-' before exponent");
            buffer += '+';
            state = State::NumberExponent;
            break;

        case State::NumberExponent:
            if (isdigit(rune))
            {
                buffer += rune;
                break;
            }

            should_reconsume = true;
            state = State::NumberDone;
            break;

        case State::NumberDone:
            value_stack.push_back(std::make_shared<Number>(atof(buffer.c_str())));
            buffer.clear();

            should_reconsume = true;
            state = return_stack.back();
            return_stack.pop_back();
            break;

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

            auto& object = value_stack.back();
            object->as<Object>()->add(key->as<String>()->get(), std::move(value));

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
            array->as<Array>()->append(std::move(value));

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
        return doc;
    }
    doc.set_root(std::move(value_stack[0]));
    return doc;
}

void Document::log_errors(std::ostream& stream)
{
    for (const auto& error : m_errors)
    {
        stream << "Error(" << error.line << ", " << error.column << ")";
        stream << ": " << error.message << "\n";
    }
}

static std::string print_indent(int indent)
{
    std::string out;
    for (int i = 0; i < indent; i++)
        out += "\t";
    return out;
}

static std::string serialize(const Value& value, bool pretty_print, int indent = 0)
{
    if (value.is<String>())
        return "\"" + value.as<Json::String>()->get() + "\"";

    if (value.is<Number>())
    {
        std::stringstream stream;
        stream << std::noshowpoint << value.to_double();
        return stream.str();
    }

    if (value.is<Boolean>())
        return value.as<Json::Boolean>()->get() ? "true" : "false";

    if (value.is<Object>())
    {
        std::string out = "{";

        auto object = value.as<Json::Object>();
        bool is_first = true;
        int new_indent = indent + (pretty_print ? 1 : 0);
        for (const auto& it : *object)
        {
            if (!is_first)
                out += ", ";

            if (pretty_print)
                out += "\n";

            out += print_indent(new_indent);
            out += "\"" + it.first + "\": ";

            if (pretty_print && (it.second->is<Object>() || it.second->is<Array>()))
                out += "\n" + print_indent(new_indent);

            out += serialize(*it.second, pretty_print,
                new_indent);

            is_first = false;
        }

        if (pretty_print)
            out += "\n" + print_indent(indent);

        return out + "}";
    }

    if (value.is<Array>())
    {
        std::string out = "[";

        auto array = value.as<Json::Array>();
        bool is_first = true;
        int new_indent = indent + (pretty_print ? 1 : 0);
        for (const auto& item : *array)
        {
            if (!is_first)
                out += ", ";

            if (pretty_print)
                out += "\n";

            out += print_indent(new_indent);
            out += serialize(*item, pretty_print, new_indent);

            is_first = false;
        }

        if (pretty_print)
            out += "\n" + print_indent(indent);

        return out + "]";
    }

    return "";
}

std::string Value::to_string(PrintOption options) const
{
    return serialize(*this, (int)options & (int)PrintOption::PrettyPrint);
}

float Value::to_float() const
{
    return static_cast<float>(as<Json::Number>()->get());
}

double Value::to_double() const
{
    return as<Json::Number>()->get();
}

int Value::to_int() const
{
    return static_cast<int>(as<Json::Number>()->get());
}

bool Value::to_bool() const
{
    return as<Json::Boolean>()->get();
}

void Object::add(const std::string& name, const std::string str)
{
    data[name] = std::make_shared<String>(str);
}

void Object::add(const std::string& name, const char* str)
{
    data[name] = std::make_shared<String>(std::string(str));
}

void Object::add(const std::string& name, double number)
{
    data[name] = std::make_shared<Number>(number);
}

void Object::add(const std::string& name, bool boolean)
{
    data[name] = std::make_shared<Boolean>(boolean);
}

std::ostream& Json::operator<<(std::ostream& out, const Value& value)
{
    out << value.to_string();
    return out;
}

std::ostream& Json::operator<<(std::ostream& out, std::shared_ptr<Value>& value)
{
    if (!value)
    {
        out << "<Null value>";
        return out;
    }

    out << *value;
    return out;
}
