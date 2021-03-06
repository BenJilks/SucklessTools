#include "database.hpp"
#include "dynamicdata.hpp"
#include "chunk.hpp"
#include "entry.hpp"
#include <cassert>
#include <cstring>
using namespace DB;

bool DataType::operator== (const DataType &other) const
{
    return m_primitive == other.m_primitive &&
        m_length == other.m_length;
}

DataType DataType::integer()
{
    return DataType(Integer, 4, 1);
}

DataType DataType::char_(int size)
{
    return DataType(Char, 1, size);
}

DataType DataType::text()
{
    return DataType(Text, 1, 1);
}

DataType DataType::big_int()
{
    return DataType(BigInt, 8, 1);
}

DataType DataType::float_()
{
    return DataType(Float, 4, 1);
}

int DataType::size_from_primitive(Primitive primitive)
{
    switch (primitive)
    {
        case Integer: return 4;
        case BigInt: return 8;
        case Float: return 4;
        case Char: return 1;
        case Text: return 1;
        default: assert (false);
    }
}

int Entry::as_int() const
{
    assert (m_data_type.primitive() == DataType::Integer);
    return static_cast<const IntegerEntry&>(*this).data();
}

int64_t Entry::as_long() const
{
    assert (m_data_type.primitive() == DataType::BigInt);
    return static_cast<const BigIntEntry&>(*this).data();
}

float Entry::as_float() const
{
    assert (m_data_type.primitive() == DataType::Float);
    return static_cast<const FloatEntry&>(*this).data();
}

std::string Entry::as_string() const
{
    auto trim = [&](auto str)
    {
        return std::string(str.data(), strlen(str.data()));
    };

    auto type = m_data_type.primitive();
    if (type == DataType::Char)
        return trim(static_cast<const CharEntry&>(*this).data());
    else if (type == DataType::Text)
        return trim(static_cast<const TextEntry&>(*this).data());

    assert (false);
}

void Entry::read(Chunk &chunk, size_t offset)
{
    m_is_null = chunk.read_byte(offset);
    read_data(chunk, offset + 1);
}

void Entry::write(Chunk &chunk, size_t offset)
{
    chunk.write_byte(offset, m_is_null);
    write_data(chunk, offset + 1);
}

template <typename T, DataType::Primitive primitive>
void TemplateEntry<T, primitive>::set(std::unique_ptr<Entry> entry)
{
    switch (entry->data_type().primitive())
    {
        case DataType::Integer:
            m_t = (T)static_cast<IntegerEntry&>(*entry).data();
            break;
        case DataType::BigInt:
            m_t = (T)static_cast<BigIntEntry&>(*entry).data();
            break;
        case DataType::Float:
            m_t = (T)static_cast<FloatEntry&>(*entry).data();
            break;
        default:
            assert (false);
    }

    m_is_null = false;
}

template <typename T, DataType::Primitive primitive>
void TemplateEntry<T, primitive>::read_data(Chunk &chunk, size_t offset)
{
    char *bytes = (char*)&m_t;
    for (int i = 0; i < (int)sizeof(T); i++)
        bytes[i] = chunk.read_byte(offset + i);
}

template <typename T, DataType::Primitive primitive>
void TemplateEntry<T, primitive>::write_data(Chunk &chunk, size_t offset)
{
    char *bytes = (char*)&m_t;
    for (int i = 0; i < (int)sizeof(T); i++)
        chunk.write_byte(offset + i, bytes[i]);
}

void CharEntry::set(std::unique_ptr<Entry> to)
{
    assert (to->data_type().primitive() == DataType::Char);

    auto other_str = static_cast<CharEntry&>(*to).data();
    assert ((int)other_str.size() <= m_size);

    memset(m_c.data(), 0, m_size);
    memcpy(m_c.data(), other_str.data(), other_str.size());
    m_is_null = false;
}

void CharEntry::read_data(Chunk &chunk, size_t offset)
{
    auto str = chunk.read_string(offset, m_size);
    memcpy(m_c.data(), str.data(), m_size);
}

void CharEntry::write_data(Chunk &chunk, size_t offset)
{
    chunk.write_string(offset, std::string(data()));
}

TextEntry::TextEntry()
    : Entry(DataType::text(), true) {}

TextEntry::TextEntry(std::string text)
    : Entry(DataType::text())
    , m_text(text)
{
    assert (false);
}

TextEntry::~TextEntry() {}

void TextEntry::set(std::unique_ptr<Entry> to)
{
    auto to_type = to->data_type().primitive();
    if (to_type == DataType::Text)
    {
        auto &other_text = static_cast<TextEntry&>(*to);
        m_dynamic_data = std::move(other_text.m_dynamic_data);
        m_text = other_text.m_text;
    }
    else if (to_type == DataType::Char)
    {
        auto &other_text = static_cast<CharEntry&>(*to);
        m_dynamic_data = nullptr;
        m_text = other_text.data();
    }
    else
    {
        assert (false);
    }

    m_is_null = false;
}

void TextEntry::read_data(Chunk &chunk, size_t offset)
{
    auto id = chunk.read_byte(offset);
    auto *table = chunk.db().find_owner(chunk.owner_id());
    assert (table);

    auto dynamic_chunk = table->find_dynamic_chunk(id);
    if (!dynamic_chunk)
        return;
    m_dynamic_data = std::make_unique<DynamicData>(dynamic_chunk);

    auto buffer = m_dynamic_data->read();
    m_text = std::string(buffer.data(), buffer.size());
}

void TextEntry::write_data(Chunk &chunk, size_t offset)
{
    if (!m_dynamic_data)
    {
        auto *table = chunk.db().find_owner(chunk.owner_id());
        assert (table);
        m_dynamic_data = table->new_dynamic_data();
    }

    std::vector<char> buffer(m_text.size());
    memcpy(buffer.data(), m_text.data(), m_text.size());
    m_dynamic_data->set(buffer);

    chunk.write_byte(offset, m_dynamic_data->id());
}

std::ostream &operator<< (std::ostream &stream, const DB::Entry& entry)
{
    if (entry.is_null())
    {
        stream << "NULL";
        return stream;
    }

    switch (entry.data_type().primitive())
    {
        case DataType::Integer:
            stream << entry.as_int();
            break;
        case DataType::BigInt:
            stream << entry.as_long();
            break;
        case DataType::Float:
            stream << entry.as_float();
            break;
        case DataType::Char:
            stream << "'" << entry.as_string() << "'";
            break;
        case DataType::Text:
            stream << "'" << entry.as_string() << "'";
            break;
        default:
            assert (false);
    }

    return stream;
}
