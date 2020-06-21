#include "database.hpp"
#include "dynamicdata.hpp"
#include "chunk.hpp"
#include "entry.hpp"
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

int DataType::size_from_primitive(Primitive primitive)
{
    switch (primitive)
    {
        case Integer: return 4;
        case Char: return 1;
        case Text: return 1;
    }
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

void IntegerEntry::set(std::unique_ptr<Entry> to)
{
    assert (to->data_type().primitive() == DataType::Integer);
    m_i = static_cast<IntegerEntry&>(*to).m_i;
    m_is_null = false;
}

void IntegerEntry::write_data(Chunk &chunk, size_t offset)
{
    chunk.write_int(offset, m_i);
}

void IntegerEntry::read_data(Chunk &chunk, size_t offset)
{
    m_i = chunk.read_int(offset);
}

void CharEntry::set(std::unique_ptr<Entry> to)
{
    assert (to->data_type().primitive() == DataType::Char);

    auto other_str = static_cast<CharEntry&>(*to).data();
    assert (other_str.size() <= m_size);

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
            stream << static_cast<const IntegerEntry&>(entry).data();
            break;
        case DataType::Char:
            stream << static_cast<const CharEntry&>(entry).data();
            break;
        case DataType::Text:
            stream << static_cast<const TextEntry&>(entry).data();
            break;
        default:
            assert (false);
    }

    return stream;
}
