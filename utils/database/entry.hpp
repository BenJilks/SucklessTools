#pragma once
#include "forward.hpp"
#include <memory>
#include <vector>
#include <string>

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
        };

        DataType(Primitive primitive, size_t size, size_t length)
            : m_primitive(primitive)
            , m_size(size)
            , m_length(length) {}

        static DataType integer();
        static DataType char_(int size);
        static DataType text();
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

    class IntegerEntry : public Entry
    {
        friend Column;

    public:
        IntegerEntry()
            : Entry(DataType::integer(), true) {}

        IntegerEntry(int i)
            : Entry(DataType::integer())
            , m_i(i) {}

        virtual void set(std::unique_ptr<Entry>) override;
        inline int data() const { return m_i; }

    private:
        virtual void read_data(Chunk &chunk, size_t offset) override;
        virtual void write_data(Chunk &chunk, size_t offset) override;

        int m_i;

    };

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
