#pragma once
#include "forward.hpp"
#include <memory>
#include <vector>
#include <string>
#include <cstring>

namespace DB
{

    class DataType
    {
    public:
        enum Primitive
        {
            Integer = 0,
            Char,
            Text,
            BigInt,
            Float,
        };

        DataType(Primitive primitive, size_t size, size_t length)
            : m_primitive(primitive)
            , m_size(size)
            , m_length(length) {}

        static DataType integer();
        static DataType char_(int size);
        static DataType text();
        static DataType big_int();
        static DataType float_();
        static int size_from_primitive(Primitive);

        inline Primitive primitive() const { return m_primitive; }
        inline size_t length() const { return m_length; }
        inline size_t size() const
        {
            // NOTE: All types start with an 'is null' flag
            return 1 + m_size * m_length;
        }

        bool operator== (const DataType &other) const;
        bool operator!= (const DataType &other) const { return !(*this == other); }

    private:
        Primitive m_primitive;
        int m_size;
        int m_length;

    };

    class Entry
    {
        friend Column;

    public:
        virtual ~Entry() = default;

        const DataType &data_type() const { return m_data_type; }
        void read(Chunk &chunk, size_t offset);
        void write(Chunk &chunk, size_t offset);
        virtual void set(std::unique_ptr<Entry>) = 0;

        int as_int() const;
        int64_t as_long() const;
        float as_float() const;
        std::string as_string() const;
        inline bool is_null() const { return m_is_null; }

    protected:
        Entry(DataType data_type, bool is_null = false)
            : m_is_null(is_null)
            , m_data_type(data_type) {}

        bool m_is_null;

    private:

        virtual void read_data(Chunk &chunk, size_t offset) = 0;
        virtual void write_data(Chunk &chunk, size_t offset) = 0;

        DataType m_data_type;

    };

    template <typename T, DataType::Primitive primitive>
    class TemplateEntry : public Entry
    {
        friend Column;

    public:
        TemplateEntry()
            : Entry(DataType(primitive, sizeof(T), 1), true) {}

        TemplateEntry(T t)
            : Entry(DataType(primitive, sizeof(T), 1))
            , m_t(t) {}

        virtual void set(std::unique_ptr<Entry>) override;

        inline T data() const { return m_t; }

    private:
        virtual void read_data(Chunk &chunk, size_t offset) override;
        virtual void write_data(Chunk &chunk, size_t offset) override;

        T m_t {};

    };

    template class TemplateEntry<int, DataType::Integer>;
    typedef TemplateEntry<int, DataType::Integer> IntegerEntry;

    template class TemplateEntry<int64_t, DataType::BigInt>;
    typedef TemplateEntry<int64_t, DataType::BigInt> BigIntEntry;

    template class TemplateEntry<float, DataType::Float>;
    typedef TemplateEntry<float, DataType::Float> FloatEntry;

    class CharEntry : public Entry
    {
        friend Column;

    public:
        CharEntry(int size)
            : Entry(DataType::char_(size), true)
            , m_c(size)
            , m_size(size) {}

        CharEntry(std::string_view str)
            : Entry(DataType::char_(str.size()))
            , m_c(str.size())
            , m_size(str.size())
        {
            memcpy(m_c.data(), str.data(), m_size);
        }

        virtual void set(std::unique_ptr<Entry>) override;
        inline std::string_view data() const { return std::string_view(m_c.data(), m_size); }

    private:
        virtual void read_data(Chunk &chunk, size_t offset) override;
        virtual void write_data(Chunk &chunk, size_t offset) override;

        std::vector<char> m_c;
        int m_size;

    };

    class TextEntry : public Entry
    {
        friend Column;

    public:
        TextEntry();
        TextEntry(std::string text);
        ~TextEntry();

        virtual void set(std::unique_ptr<Entry>) override;
        inline const std::string &data() const { return m_text; }

    private:
        virtual void read_data(Chunk &chunk, size_t offset) override;
        virtual void write_data(Chunk &chunk, size_t offset) override;

        std::unique_ptr<DynamicData> m_dynamic_data { nullptr };
        std::string m_text;

    };

}

std::ostream &operator<< (std::ostream&, const DB::Entry&);
