#ifndef EZPACKER_CODESECTION_H
#define EZPACKER_CODESECTION_H

#include "EzCodeEmitterCommon.h"

enum class SectionType : uint8_t
{
    Text,
    ReadOnly,
    ReadOnlyWithRel,
    CString,
    Const4,
    Const8,
    Const16AndBigger,
    Data,
    DataWithRel,
    NonInitialized,
    Custom
};

struct SectionFlags
{
    bool m_readable{ true };
    bool m_writable{ false };
    bool m_executable{ false };
};

enum class TargetEndianness : uint8_t
{
    Little,
    Big
};

// =========================================================================
// Node Representation
// =========================================================================
enum class SectionNodeKind : uint8_t
{
    Data,  // Chunk of emitted raw bytes
    Label, // Label bookmark / marker
    Align  // Dynamic alignment directive
};

struct SectionNode
{
    SectionNodeKind m_kind;
    SectionNode *m_prev{ nullptr };
    SectionNode *m_next{ nullptr };

    // Payload for NodeKind::Data
    std::pmr::vector<uint8_t> m_data;

    // Payload for NodeKind::Label
    MirId m_labelId{ MIRID_INVALID };
    uint64_t m_calculatedOffset{ 0 };

    // Payload for NodeKind::Align
    size_t m_alignment{ 1 };
    uint8_t m_padByte{ 0 };

    SectionNode(SectionNodeKind kind, std::pmr::memory_resource *alloc) : m_kind(kind), m_data(alloc) {}
};

/**
 * Represents an output binary section utilizing node-based linked streams.
 * Supports non-linear insertion and late serialization.
 */
class CodeSection
{
  public:
    CodeSection(SectionFlags flags,
                SectionType type,
                size_t alignment,
                TargetEndianness endianness,
                uint8_t padByte,
                std::string_view name,
                std::pmr::memory_resource *alloc);

    SectionFlags getFlags() const;
    SectionType getType() const;
    size_t getAlignment() const;

    // Node Cursor Manipulation
    SectionNode *getHead() const;
    SectionNode *getCursor() const;

    // Node Insertion
    SectionNode *bindLabel(MirId labelId);

    void alignTo(size_t alignment);

    // Emitting Operations (Writes into the active cursor block)
    void emit8(uint8_t val);
    void emit16(uint16_t val);
    void emit32(uint32_t val);
    void emit64(uint64_t val);
    void emitBytes(const uint8_t *data, size_t size);
    void emitBytesWithEndian(const uint8_t *data, size_t size, TargetEndianness inputEndianness);

    /**
     * Traverses all nodes, computes final offsets for all labels, evaluates
     * alignments, and flattens everything into m_buffer.
     */
    void finalize();

    /**
     * Resets the cursor to the end of the node list.
     */
    void resetCursorToEnd();

    /**
     * Sets the current cursor.
     */
    void setCursor(SectionNode *node);

    /**
     * Patches an offset in the flattened buffer (must be called after finalize()).
     */
    bool patch32(uint64_t offset, uint32_t val);
    bool patch64(uint64_t offset, uint64_t val);
    bool patchBytesWithEndian(uint64_t offset, const uint8_t *data, size_t size, TargetEndianness inputEndianness);

    uint64_t getCurrentOffset() const;
    std::string_view getName() const;
    std::span<const uint8_t> getData() const;

  private:
    SectionNode *createDataNode();
    SectionNode *insertNodeAfter(SectionNode *target, SectionNodeKind kind);
    std::pmr::vector<uint8_t> &getActiveDataBuffer();

  private:
    bool m_isFinalized;
    SectionFlags m_flags;

    // Linked List of Nodes
    SectionNode *m_head;
    SectionNode *m_tail;
    SectionNode *m_cursor;

    SectionType m_type;
    size_t m_alignment;
    TargetEndianness m_endianness;
    uint8_t m_padByte;

    std::string_view m_name;

    // Final Serialized Data
    std::pmr::vector<uint8_t> m_buffer;

    std::pmr::memory_resource *m_alloc;
};

#endif // EZPACKER_CODESECTION_H