#pragma once
#include "allocator.hpp"
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Json
{

    enum class PrintOption
    {
        None        = 0,
        PrettyPrint = 1 << 0,
        Serialize   = 1 << 1,
    };

#define GET_OR_NEW(type) \
    if (contains(name)) \
        return get(name); \
     \
    return (Value&)*m_allocator.make_##type;

    class Value
    {
        friend AllocatorCustom;
        friend Document;

    public:
        Value(Value &) = delete;
        Value(Value &&) = delete;
        Value(const Value &) = delete;
        Value(const Value &&) = delete;
        virtual ~Value()
        {
            m_allocator.did_delete();
        }

        virtual Value& get(const std::string&) { return *s_null_value; }
        virtual Value& get(int) { return *s_null_value; }
        virtual const Value& get(const std::string&) const { return *s_null_value; }
        virtual const Value& get(int) const { return *s_null_value; }

        inline Value &operator[] (const std::string& name) { return get(name); }
        inline Value &operator[] (size_t index) { return get(index); }
        inline const Value &operator[] (const std::string& name) const { return get(name); }
        inline const Value &operator[] (size_t index) const { return get(index); }

        inline Value &get_or_new_null(const std::string &name) { GET_OR_NEW(null()); }
        inline Value &get_or_new_object(const std::string &name) { GET_OR_NEW(object()); }
        inline Value &get_or_new_array(const std::string &name) { GET_OR_NEW(array()); }
        inline Value &get_or_new_string(const std::string &name, std::string_view str) { GET_OR_NEW(string(str)); }
        inline Value &get_or_new_number(const std::string &name, double n) { GET_OR_NEW(number(n)); }
        inline Value &get_or_new_boolean(const std::string &name, bool b) { GET_OR_NEW(boolean(b)); }

        inline Value &add_new_null(const std::string &name) { return add_new(name, m_allocator.make_null()); }
        inline Value &add_new_object(const std::string &name) { return add_new(name, m_allocator.make_object()); }
        inline Value &add_new_array(const std::string &name) { return add_new(name, m_allocator.make_array()); }
        inline Value &add_new_string(const std::string &name, std::string_view str) { return add_new(name, m_allocator.make_string(str)); }
        inline Value &add_new_number(const std::string &name, double n) { return add_new(name, m_allocator.make_number(n)); }
        inline Value &add_new_boolean(const std::string &name, bool b) { return add_new(name, m_allocator.make_boolean(b)); }

        inline Value &append_new_null() { return append_new(m_allocator.make_null()); }
        inline Value &append_new_object() { return append_new(m_allocator.make_object()); }
        inline Value &append_new_array() { return append_new(m_allocator.make_array()); }
        inline Value &append_new_string(std::string_view str) { return append_new(m_allocator.make_string(str)); }
        inline Value &append_new_number(double n) { return append_new(m_allocator.make_number(n)); }
        inline Value &append_new_boolean(bool b) { return append_new(m_allocator.make_boolean(b)); }

        virtual void add(const std::string&, const std::string) {}
        virtual void add(const std::string&, const char*) {}
        virtual void add(const std::string&, double) {}
        virtual void add(const std::string&, bool) {}
        virtual void remove(const std::string&) {}
        virtual bool contains(const std::string&) const { return false; }
        virtual void clear() {}

        virtual std::string to_string(PrintOption options = PrintOption::None) const;
        virtual std::string to_string_or(const std::string &) const { return to_string(); }
        virtual std::vector<const Value*> to_array() const { return {}; }
        virtual std::vector<std::pair<std::string, const Value*>> to_key_value_array() const { return {}; }
        virtual float to_float() const { return 0; }
        virtual double to_double() const { return 0; }
        virtual int to_int() const { return 0; }
        virtual bool to_bool() const { return false; }

        virtual bool is_null() const { return false; }
        virtual bool is_object() const { return false; }
        virtual bool is_array() const { return false; }
        virtual bool is_string() const { return false; }
        virtual bool is_number() const { return false; }
        virtual bool is_boolean() const { return false; }

    protected:
        Value(Allocator &allocator)
            : m_allocator(allocator) {}

        template<typename T>
        inline Value &add_new(const std::string& name, T *value)
        {
            add(name, value);
            return static_cast<Value&>(*value);
        }
        template<typename T>
        inline Value &append_new(T *value)
        {
            append(value);
            return static_cast<Value&>(*value);
        }

        virtual void add(const std::string&, Value*) {}
        virtual void append(Value*) {}

        Allocator &m_allocator;
        static Value *s_null_value;
    };

    class Null final : public Value
    {
        friend AllocatorCustom;

    public:
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual std::string to_string_or(const std::string &or_v) const override { return or_v; }
        virtual bool is_null() const override { return true; }

        static Null s_null_value_impl;
    private:
        explicit Null(Allocator &allocator)
            : Value(allocator) {}
    };

    class Object final : public Value
    {
        friend AllocatorCustom;

    public:
        virtual Value& get(const std::string& name) override
        {
            return !contains(name) ? *s_null_value : *m_data[name];
        }
        virtual const Value& get(const std::string& name) const override
        {
            return !contains(name) ? *s_null_value : *m_data.at(name);
        }

        virtual void add(const std::string& name, const std::string str) override;
        virtual void add(const std::string& name, const char*) override;
        virtual void add(const std::string& name, double) override;
        virtual void add(const std::string& name, bool) override;
        virtual void remove(const std::string& name) override { m_data.erase(name); }
        virtual bool contains(const std::string& name) const override { return m_data.find(name) != m_data.end(); }
	virtual void clear() override { m_data.clear(); }

        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual std::vector<std::pair<std::string, const Value*>> to_key_value_array() const override
        {
            std::vector<std::pair<std::string, const Value*>> out;
            for (const auto &value : m_data)
                out.push_back(std::make_pair(value.first, value.second));
            return out;
        }

        const auto begin() const { return m_data.begin(); }
        const auto end() const { return m_data.end(); }
        virtual bool is_object() const override { return true; }

    private:
        explicit Object(Allocator &allocator)
            : Value(allocator) { m_data.reserve(10); }

        virtual void add(const std::string& name, Value *value) override
        {
            m_data[name] = value;
        }

        std::unordered_map<std::string, Value*> m_data;
    };

    class Array final : public Value
    {
        friend AllocatorCustom;

    public:
        virtual Value& get(int index) override { return *m_data[index]; }

        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual std::vector<const Value*> to_array() const override
        {
            std::vector<const Value*> out;
            for (const auto &value : m_data)
                out.push_back(value);
            return out;
        }

        const auto begin() const { return m_data.begin(); }
        const auto end() const { return m_data.end(); }
        virtual bool is_array() const override { return true; }
	virtual void clear() override { m_data.clear(); }

    private:
        explicit Array(Allocator &allocator)
            : Value(allocator) { m_data.reserve(10); }

        virtual void append(Value *value) override
        {
            m_data.push_back(value);
        }

        std::vector<Value*> m_data;
    };

    class String final : public Value
    {
        friend AllocatorCustom;

    public:
        inline const std::string_view get_str() const { return m_data; }
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual bool is_string() const override { return true; }

    private:
        explicit String(Allocator &allocator, const std::string_view& str)
            : Value(allocator)
            , m_data(str) {}

        std::string_view m_data;
    };

    class Number final : public Value
    {
        friend AllocatorCustom;

    public:
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual float to_float() const override { return m_data; }
        virtual double to_double() const override { return m_data; }
        virtual int to_int() const override { return m_data; }
        virtual bool to_bool() const override { return m_data == 0 ? false : true; }
        virtual bool is_number() const override { return true; }

    private:
        explicit Number(Allocator &allocator, double number)
            : Value(allocator)
            , m_data(number) {}

        double m_data;
    };

    class Boolean final : public Value
    {
        friend AllocatorCustom;

    public:
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual float to_float() const override { return m_data ? 1 : 0; }
        virtual double to_double() const override { return m_data ? 1 : 0; }
        virtual int to_int() const override { return m_data ? 1 : 0; }
        virtual bool to_bool() const override { return m_data; }
        virtual bool is_boolean() const override { return true; }

    private:
        explicit Boolean(Allocator &allocator, bool boolean)
            : Value(allocator)
            , m_data(boolean) {}

        bool m_data;
    };

    struct Error
    {
        size_t line, column;
        std::string message;
    };

    class Document
    {
    public:
        static Document parse(std::istream&& stream);
        Document(Document&&) = default;
        ~Document();

        inline const Value &root() const { return *m_root; }
        inline Value &root() { return *m_root; }

        inline Value &root_or_new_object()
        {
            if (m_root->is_null())
                m_root = m_allocator->make_object();
            return *m_root;
        }
        inline Value &root_or_new_array()
        {
            if (m_root->is_null())
                m_root = m_allocator->make_object();
            return *m_root;
        }

        inline bool has_error() const { return m_errors.size() > 0; }
        void log_errors(std::ostream& stream = std::cout);

    private:
        Document(std::unique_ptr<Allocator> allocator)
            : m_allocator(std::move(allocator))
        {
            m_root = m_allocator->make_null();
        }

        inline void set_root(Value *root) { m_root = root; }
        inline void emit_error(const Error&& error) { m_errors.push_back(error); }

        std::vector<Error> m_errors;
        std::unique_ptr<Allocator> m_allocator;
        Value *m_root;
    };

    std::ostream& operator<<(std::ostream& out, const Value &value);
    std::ostream& operator<<(std::ostream& out, Value *value);

}
