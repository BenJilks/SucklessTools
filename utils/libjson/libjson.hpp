#pragma once
#include <iostream>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include "allocator.hpp"

namespace Json
{

    enum class ValueType
    {
        Object,
        Array,
        String,
        Number,
        Boolean,
        Null
    };

    enum class PrintOption
    {
        None        = 0,
        PrettyPrint = 1 << 0,
        Serialize   = 1 << 1,
    };

    class Value
    {
        friend Allocator;

    public:
        virtual ~Value() {}

        template <typename T>
        bool is() const { return this_type() == T::type(); }

        virtual Value& get(const std::string&) { return *s_null_value; }
        virtual Value& get(int) { return *s_null_value; }
        virtual Value& value_or(Value&&) { return *this; }
        virtual const Value& get(const std::string&) const { return *s_null_value; }
        virtual const Value& get(int) const { return *s_null_value; }
        virtual const Value& value_or(Value&&) const { return *this; }
        virtual void set(const std::string&, Owner<Value>) {}
        virtual void set(int, Owner<Value>) {}

        inline Value &operator[] (const std::string& name) { return get(name); }
        inline Value &operator[] (size_t index) { return get(index); }
        inline const Value &operator[] (const std::string& name) const { return get(name); }
        inline const Value &operator[] (size_t index) const { return get(index); }

        template <typename T, typename ...Args>
        inline WeakRef<T> add_new(Allocator &allocator, const std::string& name, Args ...args)
        {
            auto value = allocator.make<T>(args...);
            auto weak_ref = value.make_weak_ref();
            add(name, std::move(value));
            return weak_ref;
        }
        template <typename T, typename ...Args>
        inline WeakRef<T> append_new(Allocator &allocator, Args ...args)
        {
            auto value = allocator.make<T>(args...);
            auto weak_ref = value.make_weak_ref();
            append(std::move(value));
            return weak_ref;
        }

        virtual void add(Allocator&, const std::string&, const std::string) {}
        virtual void add(Allocator&, const std::string&, const char*) {}
        virtual void add(Allocator&, const std::string&, double) {}
        virtual void add(Allocator&, const std::string&, bool) {}
        virtual void add(const std::string&, Owner<Value>) {}
        virtual void append(Owner<Value>) {}
        virtual void remove(const std::string&) {}
        virtual bool contains(const std::string&) const { return false; }

        virtual std::string to_string(PrintOption options = PrintOption::None) const;
        virtual std::string to_string_or(const std::string &) const { return to_string(); }
        virtual std::vector<const WeakRef<Value>> to_array() const { return {}; }
        virtual float to_float() const { return 0; }
        virtual double to_double() const { return 0; }
        virtual int to_int() const { return 0; }
        virtual bool to_bool() const { return false; }

    protected:
        Value(std::shared_ptr<Allocator::MemoryChunk> memory_protector)
            : m_memory_protector(memory_protector) {}

        virtual ValueType this_type() const = 0;

        std::shared_ptr<Allocator::MemoryChunk> m_memory_protector;
        static Value *s_null_value;
    };

    class Null final : public Value
    {
        friend Allocator;

    public:
        inline static ValueType type() { return ValueType::Null; }
        virtual Value& value_or(Value&& or_v) override { return or_v; }
        virtual const Value& value_or(Value&& or_v) const override { return or_v; }
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual std::string to_string_or(const std::string &or_v) const override { return or_v; }

        static Null s_null_value_impl;
    private:
        explicit Null(std::shared_ptr<Allocator::MemoryChunk> memory_protector)
            : Value(memory_protector) {}

        virtual ValueType this_type() const override { return type(); }
    };

    class Object final : public Value
    {
        friend Allocator;

    public:
        virtual Value& get(const std::string& name) override
        {
            return !contains(name) ? *s_null_value : *m_data[name];
        }
        virtual const Value& get(const std::string& name) const override
        {
            return !contains(name) ? *s_null_value : *m_data.at(name);
        }
        virtual void set(const std::string& name, Owner<Value> value) override
        {
            m_data[name] = std::move(value);
        }

        virtual void add(Allocator&, const std::string& name, const std::string str) override;
        virtual void add(Allocator&, const std::string& name, const char*) override;
        virtual void add(Allocator&, const std::string& name, double) override;
        virtual void add(Allocator&, const std::string& name, bool) override;
        virtual void add(const std::string& name, Owner<Value> value) override
        {
            m_data[name] = std::move(value);
        }
        virtual void remove(const std::string& name) override { m_data.erase(name); }
        virtual bool contains(const std::string& name) const override { return m_data.find(name) != m_data.end(); }

        inline static ValueType type() { return ValueType::Object; }
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;

        const auto begin() const { return m_data.begin(); }
        const auto end() const { return m_data.end(); }

    private:
        explicit Object(std::shared_ptr<Allocator::MemoryChunk> memory_protector)
            : Value(memory_protector) { m_data.reserve(10); }

        virtual ValueType this_type() const override { return type(); }

        std::unordered_map<std::string, Owner<Value>> m_data;
    };

    class Array : public Value
    {
        friend Allocator;

    public:
        virtual Value& get(int index) override { return *m_data[index]; }
        virtual void set(int index, Owner<Value> value) override
        {
            if (m_data.size() <= index)
                m_data.resize(index);
            m_data[index] = std::move(value);
        }

        virtual void append(Owner<Value> value) override
        {
            m_data.push_back(std::move(value));
        }

        inline static ValueType type() { return ValueType::Array; }
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual std::vector<const WeakRef<Value>> to_array() const override
        {
            std::vector<const WeakRef<Value>> out;
            for (const auto &value : m_data)
                out.push_back(value.make_weak_ref());
            return out;
        }

        const auto begin() const { return m_data.begin(); }
        const auto end() const { return m_data.end(); }

    private:
        explicit Array(std::shared_ptr<Allocator::MemoryChunk> memory_protector)
            : Value(memory_protector) { m_data.reserve(10); }

        virtual ValueType this_type() const override { return type(); }

        std::vector<Owner<Value>> m_data;
    };

    class String : public Value
    {
        friend Allocator;

    public:
        inline static ValueType type() { return ValueType::String; }
        inline const std::string_view get_str() const { return m_data; }
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;

    private:
        explicit String(std::shared_ptr<Allocator::MemoryChunk> memory_protector, const std::string_view& str)
            : Value(memory_protector)
            , m_data(str) {}

        virtual ValueType this_type() const override { return type(); }

        std::string_view m_data;
    };

    class Number : public Value
    {
        friend Allocator;

    public:
        inline static ValueType type() { return ValueType::Number; }
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual float to_float() const override { return m_data; }
        virtual double to_double() const override { return m_data; }
        virtual int to_int() const override { return m_data; }
        virtual bool to_bool() const override { return m_data == 0 ? false : true; }

    private:
        explicit Number(std::shared_ptr<Allocator::MemoryChunk> memory_protector, double number)
            : Value(memory_protector)
            , m_data(number) {}

        virtual ValueType this_type() const override { return type(); }

        double m_data;
    };

    class Boolean : public Value
    {
        friend Allocator;

    public:
        inline static ValueType type() { return ValueType::Boolean; }
        virtual std::string to_string(PrintOption options = PrintOption::None) const override;
        virtual float to_float() const override { return m_data ? 1 : 0; }
        virtual double to_double() const override { return m_data ? 1 : 0; }
        virtual int to_int() const override { return m_data ? 1 : 0; }
        virtual bool to_bool() const override { return m_data; }

    private:
        explicit Boolean(std::shared_ptr<Allocator::MemoryChunk> memory_protector, bool boolean)
            : Value(memory_protector)
            , m_data(boolean) {}

        virtual ValueType this_type() const override { return type(); }

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

        inline Allocator &allocator() { return *m_allocator; }
        inline const Value &root() const { return *m_root; }
        inline Value &root() { return *m_root; }

        inline bool has_error() const { return m_errors.size() > 0; }
        void log_errors(std::ostream& stream = std::cout);

    private:
        Document()
            : m_allocator(std::make_unique<Allocator>())
        {
            m_root = m_allocator->make<Null>();
        }

        inline void set_root(Owner<Value> root) { m_root = std::move(root); }
        inline void emit_error(const Error&& error) { m_errors.push_back(error); }

        std::vector<Error> m_errors;
        std::unique_ptr<Allocator> m_allocator;
        Owner<Value> m_root;
    };

    std::ostream& operator<<(std::ostream& out, const Value& value);
    std::ostream& operator<<(std::ostream& out, Owner<Value>& value);

}
