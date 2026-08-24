#ifndef EZPACKER_CODESECTION_H
#define EZPACKER_CODESECTION_H

#include "EzCodeEmitterCommon.h"

/**
 * Categorization of binary output sections for object file formats.
 */
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

/**
 * Access permissions for a code or data section.
 */
struct SectionFlags
{
    bool m_readable{ true };
    bool m_writable{ false };
    bool m_executable{ false };
};

/**
 * Byte-order orientation for data emission.
 */
enum class TargetEndianness : uint8_t
{
    Little,
    Big
};

// =========================================================================
// Node Representation
// =========================================================================

/**
 * Discriminant tag identifying the kind of node in a section's linked stream.
 */
enum class SectionNodeKind : uint8_t
{
    Data,  // Chunk of emitted raw bytes
    Label, // Label bookmark / marker
    Align  // Dynamic alignment directive
};

/**
 * Node element within a CodeSection doubly-linked stream allowing non-linear insertion and late layout evaluation.
 */
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
    /**
     * Constructs a code section with flags, type classification, alignment, endianness, padding byte, and name.
     */
    CodeSection(SectionFlags flags,
                SectionType type,
                size_t alignment,
                TargetEndianness endianness,
                uint8_t padByte,
                std::string_view name,
                std::pmr::memory_resource *alloc);

    /**
     * Returns the memory access permission flags of the section.
     */
    SectionFlags getFlags() const;

    /**
     * Returns the section's type classification.
     */
    SectionType getType() const;

    /**
     * Returns the byte alignment constraint required for this section.
     */
    size_t getAlignment() const;

    // Node Cursor Manipulation

    /**
     * Returns the head node of the section's linked node stream.
     */
    SectionNode *getHead() const;

    /**
     * Returns the current active insertion cursor node.
     */
    SectionNode *getCursor() const;

    // Node Insertion

    /**
     * Binds a label marker node at the current cursor position.
     */
    SectionNode *bindLabel(MirId labelId);

    /**
     * Inserts an alignment directive node at the current cursor position.
     */
    void alignTo(size_t alignment);

    // Emitting Operations (Writes into the active cursor block)

    /**
     * Emits an 8-bit unsigned integer into the active data buffer.
     */
    void emit8(uint8_t val);

    /**
     * Emits a 16-bit integer respecting the section's target endianness.
     */
    void emit16(uint16_t val);

    /**
     * Emits a 32-bit integer respecting the section's target endianness.
     */
    void emit32(uint32_t val);

    /**
     * Emits a 64-bit integer respecting the section's target endianness.
     */
    void emit64(uint64_t val);

    /**
     * Emits raw byte data into the active data buffer.
     */
    void emitBytes(const uint8_t *data, size_t size);

    /**
     * Emits raw byte data, swapping endianness if inputEndianness differs from target endianness.
     */
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

    /**
     * Returns the current cumulative byte offset in the section.
     */
    uint64_t getCurrentOffset() const;

    /**
     * Returns the section name.
     */
    std::string_view getName() const;

    /**
     * Returns a view over the serialized byte buffer (available after finalize()).
     */
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