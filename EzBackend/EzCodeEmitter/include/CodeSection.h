#ifndef EZPACKER_CODESECTION_H
#define EZPACKER_CODESECTION_H

#include "EzCodeEmitterCommon.h"

enum class SectionType : uint8_t
{
    Text,

    /**
     * Read-only data sections subtypes. Important:
     *  - Some sections may not be used (imagine an executable without 16byte, or bigger, constants).
     *  - 1 or more section may be merged into a single container section.
     */

    ReadOnly,         // Generic structs, jump tables, VTables without relocs
    ReadOnlyWithRel,  // Read-only data containing pointer relocations (needs runtime fixup)
    CString,          // Section with Null-terminated string literals ('\0'-terminated)
    Const4,           // Section with 4-byte scalar float-int constants
    Const8,           // Section with 8-byte scalar double-int constants
    Const16AndBigger, // Section with 16-byte SIMD / 128-bit constants and bigger sizes (256, 512, ...).

    // Mutable & Uninitialized Data
    Data,        // Mutable initialized globals
    DataWithRel, // Mutable globals initialized with pointers/addresses
    NonInitialized,         // Uninitialized zero-fill memory

    // Target / Custom (for ex: .eh_frame)
    Custom
};

struct SectionFlags
{
    bool m_readable{ true }; // Read by default.
    bool m_writable{ false };
    bool m_executable{ false };
};

enum class TargetEndianness : uint8_t
{
    Little, // The first bit is the LSB (Least Significant Bit).
    Big     // The first bit is the MSB (Most Significant Bit).
};

/**
 * Represents a single output binary section maintaining its own byte buffer,
 * alignment constraints, and section-local offset tracking.
 */
class CodeSection
{
  public:
    /**
     * Creates a section with the given parameters.
     */
    CodeSection(SectionFlags flags,
                SectionType type,
                size_t alignment,
                TargetEndianness endianness,
                uint8_t padByte,
                std::string_view name,
                std::pmr::memory_resource *alloc);

    /**
     * Returns the flags of this section.
     */
    SectionFlags getFlags() const;

    /**
     * Returns the type of the section.
     */
    SectionType getType() const;

    /**
     * Returns the alignment for the section.
     */
    size_t getAlignment() const;

    /**
     * Returns the current offset the section has been stopped into.
     */
    uint64_t getCurrentOffset() const;

    /**
     * Emits a single byte into the section. Takes in account ENDIANNES.
     */
    void emit8(uint8_t val);

    /**
     * Emits a word into the section. Takes in account ENDIANNES.
     */
    void emit16(uint16_t val);

    /**
     * Emits a double word into the section. Takes in account ENDIANNES.
     */
    void emit32(uint32_t val);

    /**
     * Emits a quadword into the section. Takes in account ENDIANNES.
     */
    void emit64(uint64_t val);

    /**
     * Emits a sequence of bytes into the section WITHOUT taking in account ENDIANNESS.
     */
    void emitBytes(const uint8_t *data, size_t size);

    /**
     * Emits the given buffer taking in account the endiannes. It will convert input's endianness to target's.
     */
    void emitBytesWithEndian(const uint8_t *data, size_t size, TargetEndianness inputEndianness);

    /**
     * Aligns the section to the current alignment using the padding byte specified in the creation of the section as
     * padding.
     */
    void alignTo(size_t alignment);

    /**
     * Patches the given offset (from the start of the section). If (offset + 4 >= sectionEnd) an error is pushed into
     * the diagnostic collector and false is returned.
     *
     * If the patch was applied properly, true is returned.
     */
    bool patch32(uint64_t offset, uint32_t val);

    /**
     * Patches the given offset (from the start of the section). If (offset + 8 >= sectionEnd) an error is pushed into
     * the diagnostic collector and false is returned.
     *
     * If the patch was applied properly, true is returned.
     */
    bool patch64(uint64_t offset, uint64_t val);

    /**
     * Patches a sequence of scalar bytes converting from inputEndianness to target endianness. If
     * (offset + size >= sectionEnd), the fnuction returns false.
     *
     * If the patch was applied properly, true is returned.
     */
    bool patchBytesWithEndian(uint64_t offset, const uint8_t *data, size_t size, TargetEndianness inputEndianness);

    std::string_view getName() const;

    std::span<const uint8_t> getData() const;

  private:
    SectionFlags m_flags;
    SectionType m_type;
    TargetEndianness m_endianness;
    size_t m_alignment;
    uint8_t m_padByte;
    std::string_view m_name;
    std::pmr::vector<uint8_t> m_buffer;
};

#endif // EZPACKER_CODESECTION_H