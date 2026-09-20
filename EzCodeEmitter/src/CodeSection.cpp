#include "CodeSection.h"

CodeSection::CodeSection(SectionFlags flags,
                         SectionType type,
                         size_t alignment,
                         TargetEndianness endianness,
                         uint8_t padByte,
                         std::string_view name,
                         std::pmr::memory_resource *alloc) :
    m_isFinalized(false), m_flags(flags), m_head(nullptr), m_tail(nullptr), m_cursor(nullptr), m_type(type),
    m_alignment(alignment), m_endianness(endianness), m_padByte(padByte), m_name(name), m_buffer(alloc), m_alloc(alloc)
{
    // Seed the stream with a single empty data node so emits always have a target.
    m_head = createDataNode();
    m_tail = m_head;
    m_cursor = m_head;
}

SectionFlags CodeSection::getFlags() const { return m_flags; }

SectionType CodeSection::getType() const { return m_type; }

size_t CodeSection::getAlignment() const { return m_alignment; }

SectionNode *CodeSection::getHead() const { return m_head; }

SectionNode *CodeSection::getCursor() const { return m_cursor; }

SectionNode *CodeSection::bindLabel(MirId labelId)
{
    // Insert a marker node; its offset is resolved later during finalize().
    auto *lblNode = insertNodeAfter(m_cursor, SectionNodeKind::Label);
    lblNode->m_labelId = labelId;
    return lblNode;
}

void CodeSection::alignTo(size_t alignment)
{
    // Record the requested alignment as a node so it is evaluated at finalize() time.
    auto *alignNode = insertNodeAfter(m_cursor, SectionNodeKind::Align);
    alignNode->m_alignment = alignment;
    alignNode->m_padByte = m_padByte;
}

void CodeSection::emit8(uint8_t val) { getActiveDataBuffer().push_back(val); }

void CodeSection::emit16(uint16_t val)
{
    auto &buf = getActiveDataBuffer();
    if (m_endianness == TargetEndianness::Little)
    {
        buf.push_back(static_cast<uint8_t>(val));
        buf.push_back(static_cast<uint8_t>(val >> 8));
    }
    else
    {
        buf.push_back(static_cast<uint8_t>(val >> 8));
        buf.push_back(static_cast<uint8_t>(val));
    }
}

void CodeSection::emit32(uint32_t val)
{
    auto &buf = getActiveDataBuffer();
    if (m_endianness == TargetEndianness::Little)
    {
        buf.push_back(static_cast<uint8_t>(val));
        buf.push_back(static_cast<uint8_t>(val >> 8));
        buf.push_back(static_cast<uint8_t>(val >> 16));
        buf.push_back(static_cast<uint8_t>(val >> 24));
    }
    else
    {
        buf.push_back(static_cast<uint8_t>(val >> 24));
        buf.push_back(static_cast<uint8_t>(val >> 16));
        buf.push_back(static_cast<uint8_t>(val >> 8));
        buf.push_back(static_cast<uint8_t>(val));
    }
}

void CodeSection::emit64(uint64_t val)
{
    auto &buf = getActiveDataBuffer();
    if (m_endianness == TargetEndianness::Little)
    {
        for (int i = 0; i < 8; ++i)
        {
            buf.push_back(static_cast<uint8_t>(val >> (i * 8)));
        }
    }
    else
    {
        for (int i = 7; i >= 0; --i)
        {
            buf.push_back(static_cast<uint8_t>(val >> (i * 8)));
        }
    }
}

void CodeSection::emitBytes(const uint8_t *data, size_t size)
{
    if (!data || size == 0)
    {
        return;
    }
    auto &buf = getActiveDataBuffer();
    buf.insert(buf.end(), data, data + size);
}

void CodeSection::emitBytesWithEndian(const uint8_t *data, size_t size, TargetEndianness inputEndianness)
{
    if (!data || size == 0)
    {
        return;
    }
    auto &buf = getActiveDataBuffer();
    buf.reserve(buf.size() + size);

    if (inputEndianness == m_endianness)
    {
        // Same byte order: copy verbatim.
        buf.insert(buf.end(), data, data + size);
    }
    else
    {
        // Opposite byte order: emit the bytes in reverse so scalars are byte-swapped.
        for (size_t i = size; i > 0; --i)
        {
            buf.push_back(data[i - 1]);
        }
    }
}

void CodeSection::finalize()
{
    m_buffer.clear();
    uint64_t currentOffset = 0;

    // Single pass over the stream: resolve label offsets and expand alignment padding
    // while concatenating all data chunks into the flattened output buffer.
    for (SectionNode *node = m_head; node != nullptr; node = node->m_next)
    {
        switch (node->m_kind)
        {
            case SectionNodeKind::Label:
                node->m_calculatedOffset = currentOffset;
                break;

            case SectionNodeKind::Align:
            {
                if (node->m_alignment > 1)
                {
                    // Round the cursor up to the next multiple of the alignment.
                    size_t rem = currentOffset % node->m_alignment;
                    if (rem != 0)
                    {
                        size_t padSize = node->m_alignment - rem;
                        m_buffer.insert(m_buffer.end(), padSize, node->m_padByte);
                        currentOffset += padSize;
                    }
                }
                break;
            }

            case SectionNodeKind::Data:
                if (!node->m_data.empty())
                {
                    m_buffer.insert(m_buffer.end(), node->m_data.begin(), node->m_data.end());
                    currentOffset += node->m_data.size();
                }
                break;
        }
    }

    m_isFinalized = true;
}

void CodeSection::resetCursorToEnd() { m_cursor = m_tail; }

void CodeSection::setCursor(SectionNode *node) { m_cursor = node ? node : m_tail; }

bool CodeSection::patch32(uint64_t offset, uint32_t val)
{
    // Patching is only valid on the flattened buffer, so require finalize() first.
    if (!m_isFinalized || (offset + 4 > m_buffer.size()))
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
    if (!m_isFinalized || (offset + 8 > m_buffer.size()))
    {
        return false;
    }

    if (m_endianness == TargetEndianness::Little)
    {
        for (int i = 0; i < 8; ++i)
        {
            m_buffer[offset + i] = static_cast<uint8_t>(val >> (i * 8));
        }
    }
    else
    {
        for (int i = 0; i < 8; ++i)
        {
            m_buffer[offset + i] = static_cast<uint8_t>(val >> ((7 - i) * 8));
        }
    }

    return true;
}

bool CodeSection::patchBytesWithEndian(uint64_t offset,
                                       const uint8_t *data,
                                       size_t size,
                                       TargetEndianness inputEndianness)
{
    if (!m_isFinalized || !data || (offset + size > m_buffer.size()))
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
        // Reverse the patch bytes when the source and section byte orders differ.
        for (size_t i = 0; i < size; ++i)
        {
            m_buffer[offset + i] = data[size - 1 - i];
        }
    }

    return true;
}

uint64_t CodeSection::getCurrentOffset() const
{
    if (m_isFinalized)
    {
        return m_buffer.size();
    }

    // Pre-finalize the offset is the effect of every node up to and including the cursor,
    // applying alignment padding the same way finalize() does so pending Align nodes are counted.
    uint64_t sz = 0;
    for (SectionNode *n = m_head; n != nullptr; n = n->m_next)
    {
        if (n->m_kind == SectionNodeKind::Align)
        {
            if (n->m_alignment > 1)
            {
                uint64_t rem = sz % n->m_alignment;
                if (rem != 0)
                {
                    sz += n->m_alignment - rem;
                }
            }
        }
        else if (n->m_kind == SectionNodeKind::Data)
        {
            sz += n->m_data.size();
        }

        if (n == m_cursor)
        {
            break;
        }
    }
    return sz;
}

std::string_view CodeSection::getName() const { return m_name; }

std::span<const uint8_t> CodeSection::getData() const { return m_buffer; }

std::span<uint8_t> CodeSection::getMutableData() { return std::span<uint8_t>(m_buffer.data(), m_buffer.size()); }

SectionNode *CodeSection::createDataNode()
{
    void *mem = m_alloc->allocate(sizeof(SectionNode), alignof(SectionNode));
    return new (mem) SectionNode(SectionNodeKind::Data, m_alloc);
}

SectionNode *CodeSection::insertNodeAfter(SectionNode *target, SectionNodeKind kind)
{
    void *mem = m_alloc->allocate(sizeof(SectionNode), alignof(SectionNode));
    auto *newNode = new (mem) SectionNode(kind, m_alloc);

    if (!target)
    {
        // Empty/anchor insertion: link the new node in front of the current head.
        newNode->m_next = m_head;
        if (m_head)
        {
            m_head->m_prev = newNode;
        }
        m_head = newNode;
        if (!m_tail)
        {
            m_tail = newNode;
        }
    }
    else
    {
        // Standard insertion immediately after target, repairing prev/next and the tail.
        newNode->m_next = target->m_next;
        newNode->m_prev = target;
        if (target->m_next)
        {
            target->m_next->m_prev = newNode;
        }
        else
        {
            m_tail = newNode;
        }
        target->m_next = newNode;
    }

    m_cursor = newNode;
    return newNode;
}

std::pmr::vector<uint8_t> &CodeSection::getActiveDataBuffer()
{
    // Reuse the cursor's buffer when it is already a data node, otherwise start a new one.
    if (m_cursor && m_cursor->m_kind == SectionNodeKind::Data)
    {
        return m_cursor->m_data;
    }
    auto *dataNode = insertNodeAfter(m_cursor, SectionNodeKind::Data);
    return dataNode->m_data;
}