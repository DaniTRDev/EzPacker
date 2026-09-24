#ifndef EZPACKER_CODESECTION_H
#define EZPACKER_CODESECTION_H

#include "EzCodeEmitterCommon.h"

/**
 * Categorization of binary output sections for object file formats.
 */
enum class SectionType : uint8_t
{
    Text,             ///< Executable machine code (".text").
    ReadOnly,         ///< Read-only constants without relocations (".rodata").
    ReadOnlyWithRel,  ///< Read-only data that still requires relocations (".data.rel.ro").
    CString,          ///< Null-terminated string literals.
    Const4,           ///< 4-byte scalar constants (floats/integers).
    Const8,           ///< 8-byte scalar constants (doubles/integers).
    Const16AndBigger, ///< 16-byte-or-larger constants (vectors/SIMD).
    Data,             ///< Mutable initialized data (".data").
    DataWithRel,      ///< Mutable data that requires relocations.
    NonInitialized,   ///< Zero-initialized storage with no file bytes (".bss").
    Custom,           ///< Target-specific metadata/exception tables.
    Undefined         ///< Symbol is not defined in this module (SHN_UNDEF / COFF section 0).
};

/**
 * Access permissions for a code or data section.
 */
struct SectionFlags
{
    bool m_readable{ true };    ///< True when the section contents may be read at runtime.
    bool m_writable{ false };   ///< True when the section contents may be modified at runtime.
    bool m_executable{ false }; ///< True when the section may be executed as code.
};

/**
 * Byte-order orientation for data emission.
 */
enum class TargetEndianness : uint8_t
{
    Little, ///< Least-significant byte emitted first (x86/x86-64).
    Big     ///< Most-significant byte emitted first.
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
    SectionNodeKind m_kind;         ///< Discriminant selecting which payload below is active.
    SectionNode *m_prev{ nullptr }; ///< Previous node in the doubly-linked stream (nullptr at head).
    SectionNode *m_next{ nullptr }; ///< Next node in the doubly-linked stream (nullptr at tail).

    // Payload for NodeKind::Data
    std::pmr::vector<uint8_t> m_data; ///< Raw bytes accumulated for this data chunk.

    // Payload for NodeKind::Label
    MirId m_labelId{ MIRID_INVALID }; ///< MIR id of the label marker.
    uint64_t m_calculatedOffset{ 0 }; ///< Byte offset resolved during finalize().

    // Payload for NodeKind::Align
    size_t m_alignment{ 1 }; ///< Required power-of-two alignment in bytes.
    uint8_t m_padByte{ 0 };  ///< Fill byte inserted to satisfy the alignment.

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
     * Returns the cumulative byte offset where the next emitted byte lands, applying any pending
     * alignment padding exactly as finalize() will.
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

    /**
     * Returns a mutable view over the serialized byte buffer, used for in-place relocation
     * patching (available after finalize()).
     */
    std::span<uint8_t> getMutableData();

  private:
    /**
     * Allocates and constructs a fresh Data node from the section allocator.
     */
    SectionNode *createDataNode();

    /**
     * Allocates a node of the given kind and links it immediately after target
     * (or at the head when target is nullptr), advancing the cursor to the new node.
     */
    SectionNode *insertNodeAfter(SectionNode *target, SectionNodeKind kind);

    /**
     * Returns the cursor's data buffer, inserting a new Data node first when the
     * cursor does not already point at one.
     */
    std::pmr::vector<uint8_t> &getActiveDataBuffer();

    /**
     * Recomputes m_cursorOffset by replaying the node stream from the head up to and including
     * node. Only needed when the cursor is moved to an arbitrary node; normal emission updates
     * the running offset incrementally.
     */
    uint64_t computeOffsetTo(const SectionNode *node) const;

  private:
    bool m_isFinalized;   ///< True once finalize() has flattened the node stream.
    SectionFlags m_flags; ///< Runtime access permissions of the section.

    // Linked List of Nodes
    SectionNode *m_head;   ///< First node of the stream.
    SectionNode *m_tail;   ///< Last node of the stream.
    SectionNode *m_cursor; ///< Node where subsequent emits/insertions take place.

    uint64_t m_cursorOffset{ 0 }; ///< Running byte offset of m_cursor, kept in sync with finalize().

    SectionType m_type;            ///< Output section classification.
    size_t m_alignment;            ///< Alignment applied when the section is laid out.
    TargetEndianness m_endianness; ///< Byte order used by the multi-byte emit helpers.
    uint8_t m_padByte;             ///< Fill byte used for alignment and padding.

    std::string_view m_name; ///< Non-owning section name (e.g. ".text").

    // Final Serialized Data
    std::pmr::vector<uint8_t> m_buffer; ///< Flattened bytes produced by finalize().

    std::pmr::memory_resource *m_alloc; ///< Allocator used for nodes and buffers.
};

#endif // EZPACKER_CODESECTION_H