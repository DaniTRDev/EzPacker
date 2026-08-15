#include "CodeSection.h"

CodeSection::CodeSection(SectionFlags flags,
                         SectionType type,
                         size_t alignment,
                         TargetEndianness endianness,
                         uint8_t padByte,
                         std::string_view name,
                         std::pmr::memory_resource *alloc) :
    m_flags(flags), m_type(type), m_alignment(alignment), m_endianness(endianness), m_padByte(padByte), m_name(name),
    m_buffer(alloc)
{
}

SectionFlags CodeSection::getFlags() const { return m_flags; }

SectionType CodeSection::getType() const { return m_type; }

size_t CodeSection::getAlignment() const { return m_alignment; }

uint64_t CodeSection::getCurrentOffset() const { return m_buffer.size(); }

void CodeSection::emit8(uint8_t val) { m_buffer.push_back(val); }

void CodeSection::emit16(uint16_t val)
{
    if (m_endianness == TargetEndianness::Little)
    {
        m_buffer.push_back(static_cast<uint8_t>(val));
        m_buffer.push_back(static_cast<uint8_t>(val >> 8));
    }
    else
    {
        m_buffer.push_back(static_cast<uint8_t>(val >> 8));
        m_buffer.push_back(static_cast<uint8_t>(val));
    }
}

void CodeSection::emit32(uint32_t val)
{
    if (m_endianness == TargetEndianness::Little)
    {
        m_buffer.push_back(static_cast<uint8_t>(val));
        m_buffer.push_back(static_cast<uint8_t>(val >> 8));
        m_buffer.push_back(static_cast<uint8_t>(val >> 16));
        m_buffer.push_back(static_cast<uint8_t>(val >> 24));
    }
    else
    {
        m_buffer.push_back(static_cast<uint8_t>(val >> 24));
        m_buffer.push_back(static_cast<uint8_t>(val >> 16));
        m_buffer.push_back(static_cast<uint8_t>(val >> 8));
        m_buffer.push_back(static_cast<uint8_t>(val));
    }
}

void CodeSection::emit64(uint64_t val)
{
    if (m_endianness == TargetEndianness::Little)
    {
        for (int i = 0; i < 8; ++i)
            m_buffer.push_back(static_cast<uint8_t>(val >> (i * 8)));
    }
    else
    {
        for (int i = 7; i >= 0; --i)
            m_buffer.push_back(static_cast<uint8_t>(val >> (i * 8)));
    }
}

void CodeSection::emitBytes(const uint8_t *data, size_t size)
{
    if (!data || size == 0)
        return;

    m_buffer.insert(m_buffer.end(), data, data + size);
}

void CodeSection::emitBytesWithEndian(const uint8_t *data, size_t size, TargetEndianness inputEndianness)
{
    if (!data || size == 0)
    {
        return;
    }

    m_buffer.reserve(m_buffer.size() + size);

    // If input and target match: emit in forward order (as-is)
    if (inputEndianness == m_endianness)
    {
        m_buffer.insert(m_buffer.end(), data, data + size);
    }
    else
    {
        // Endianness differs: reverse the byte sequence
        for (size_t i = size; i > 0; --i)
        {
            m_buffer.push_back(data[i - 1]);
        }
    }
}

void CodeSection::alignTo(size_t alignment)
{
    size_t current = m_buffer.size();
    size_t rem = current % alignment;
    if (rem != 0)
    {
        size_t padSize = alignment - rem;
        m_buffer.insert(m_buffer.end(), padSize, m_padByte);
    }
}

bool CodeSection::patch32(uint64_t offset, uint32_t val)
{
    // Bounds check for 4 bytes
    if (offset + 4 > m_buffer.size())
    {
        return false;
    }

    if (m_endianness == TargetEndianness::Little)
    {
        m_buffer[offset + 0] = static_cast<uint8_t>(val);
        m_buffer[offset + 1] = static_cast<uint8_t>(val >> 8);
        m_buffer[offset + 2] = static_cast<uint8_t>(val >> 16);
        m_buffer[offset + 3] = static_cast<uint8_t>(val >> 24);
    }
    else
    {
        m_buffer[offset + 0] = static_cast<uint8_t>(val >> 24);
        m_buffer[offset + 1] = static_cast<uint8_t>(val >> 16);
        m_buffer[offset + 2] = static_cast<uint8_t>(val >> 8);
        m_buffer[offset + 3] = static_cast<uint8_t>(val);
    }

    return true;
}

bool CodeSection::patch64(uint64_t offset, uint64_t val)
{
    // Bounds check for 8 bytes
    if (offset + 8 > m_buffer.size())
    {
        return false;
    }

    if (m_endianness == TargetEndianness::Little)
    {
        m_buffer[offset + 0] = static_cast<uint8_t>(val);
        m_buffer[offset + 1] = static_cast<uint8_t>(val >> 8);
        m_buffer[offset + 2] = static_cast<uint8_t>(val >> 16);
        m_buffer[offset + 3] = static_cast<uint8_t>(val >> 24);
        m_buffer[offset + 4] = static_cast<uint8_t>(val >> 32);
        m_buffer[offset + 5] = static_cast<uint8_t>(val >> 40);
        m_buffer[offset + 6] = static_cast<uint8_t>(val >> 48);
        m_buffer[offset + 7] = static_cast<uint8_t>(val >> 56);
    }
    else
    {
        m_buffer[offset + 0] = static_cast<uint8_t>(val >> 56);
        m_buffer[offset + 1] = static_cast<uint8_t>(val >> 48);
        m_buffer[offset + 2] = static_cast<uint8_t>(val >> 40);
        m_buffer[offset + 3] = static_cast<uint8_t>(val >> 32);
        m_buffer[offset + 4] = static_cast<uint8_t>(val >> 24);
        m_buffer[offset + 5] = static_cast<uint8_t>(val >> 16);
        m_buffer[offset + 6] = static_cast<uint8_t>(val >> 8);
        m_buffer[offset + 7] = static_cast<uint8_t>(val);
    }

    return true;
}

bool CodeSection::patchBytesWithEndian(uint64_t offset,
                                       const uint8_t *data,
                                       size_t size,
                                       TargetEndianness inputEndianness)
{
    if (!data || (offset + size > m_buffer.size()))
    {
        return false;
    }

    if (inputEndianness == m_endianness)
    {
        for (size_t i = 0; i < size; ++i)
        {
            m_buffer[offset + i] = data[i];
        }
    }
    else
    {
        for (size_t i = 0; i < size; ++i)
        {
            m_buffer[offset + i] = data[size - 1 - i];
        }
    }

    return true;
}

std::string_view CodeSection::getName() const { return m_name; }

std::span<const uint8_t> CodeSection::getData() const { return m_buffer; }