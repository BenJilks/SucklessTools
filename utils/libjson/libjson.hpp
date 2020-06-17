#pragma once
#include <string>
#include <vector>
#include <map>

namespace Json
{

    class Value
    {
    public:
        Value()
            : m_type(Null) {}

        enum Type
        {
            Null,
            Object,
            Array,
            String,
            Number,
            Boolean
        };

        Value &operator[] (const std::string &name);
        const Value &operator[] (const std::string &name) const;
        Value &append(Value);
        void clear();

        static Value parse(std::istream &&stream);
        static Value null();
        static Value object();
        static Value array();
        static Value string(const std::string&);
        static Value number(double);
        static Value boolean(bool);

        inline bool is_null() const { return m_type == Null; }
        inline bool is_object() const { return m_type == Object; }
        inline bool is_array() const { return m_type == Array; }
        inline bool is_string() const { return m_type == String; }
        inline bool is_number() const { return m_type == Number; }
        inline bool is_boolean() const { return m_type == Boolean; }

        std::vector<std::pair<std::string, Value>> as_key_value_pairs() const;
        std::vector<Value> as_array() const;
        std::string as_string(bool include_quotes = false) const;
        double as_number() const;
        bool as_boolean() const;

        std::string pretty_print(int indent = 0) const;
        Value &otherwise(Value&&);

    private:
        explicit Value(Type type)
            : m_type(type) {}
        explicit Value(const std::string &str)
            : m_type(String)
            , m_string(str) {}
        explicit Value(double number)
            : m_type(Number)
            , m_number(number) {}
        explicit Value(bool boolean)
            : m_type(Boolean)
            , m_boolean(boolean) {}

        Type m_type;

        double m_number;
        bool m_boolean;
        std::string m_string;
        std::map<std::string, Value> m_members;
        std::vector<Value> m_elements;
    };

    static Value parse(std::istream &&stream)
    {
        return Value::parse(std::move(stream));
    };

}

std::ostream &operator<< (std::ostream&, const Json::Value&);
