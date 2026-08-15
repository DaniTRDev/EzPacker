#ifndef EZPACKER_TARGETBINARYDESC_H
#define EZPACKER_TARGETBINARYDESC_H

#include "EzTripleCommon.h"

enum class CodeModel : uint8_t
{
    Small, // Global addresses can be encoded in instruction pointer + small integer immediate.
    Large  // The entire address must be encoded in the instruction.
};

enum class Endianness : uint8_t
{
    Little, // The first bit is the LSB (Least Significant Bit).
    Big     // The first bit is the MSB (Most Significant Bit).
};

enum class ObjectFormat : uint8_t
{
    ELF,
    COFF,
    MachO
};

/**
 * Target-agnostic relocation types representing standard relocation fixups
 * required across various object formats (ELF, COFF/PE, Mach-O).
 */
enum class CodeRelocationType : uint8_t
{
    /** No relocation needed or relocation is undefined. */
    None,

    /**
     * Direct 32-bit absolute address fixup (e.g., ELF R_X86_64_32 / COFF IMAGE_REL_AMD64_ADDR32).
     * The linker writes the full 32-bit virtual address of the symbol directly into the field.
     */
    Absolute32,

    /**
     * Direct 64-bit absolute address fixup (e.g., ELF R_X86_64_64 / COFF IMAGE_REL_AMD64_ADDR64).
     * Used by instructions loading 64-bit immediate pointers (such as x86-64 `movabs reg, imm64`).
     */
    Absolute64,

    /**
     * 32-bit signed PC-relative (RIP-relative) data displacement (e.g., ELF R_X86_64_PC32 / COFF
     * IMAGE_REL_AMD64_REL32). Computes the signed offset between the next instruction address (PC/RIP) and the target
     * symbol (e.g., `mov reg, [rip + symbol]`).
     */
    PCRel32,

    /**
     * 32-bit signed PC-relative branch or call offset (e.g., ELF R_X86_64_PLT32 or direct branch relocations).
     * Used specifically for control flow instructions (`call target`, `jmp target`, conditional jumps).
     */
    BranchRel32,

    /**
     * 32-bit PC-relative reference to a Global Offset Table (GOT) entry (e.g., ELF R_X86_64_GOTPCREL).
     * Used in Position-Independent Code (PIC) to resolve external/global symbols via an indirect pointer in the GOT.
     */
    GOTPCREL,

    /**
     * 32-bit PC-relative reference to a Procedure Linkage Table (PLT) entry (e.g., ELF R_X86_64_PLT32).
     * Used in shared libraries and dynamic linking to invoke external functions via dynamic stubs.
     */
    PLTRel32
};

/**
 * Interface that contains information about a target ABI (OS-LEVEL). Ex: TargetDesc = AMD64, TargetBinaryDesc =
 * AMD64_Windows | AMD64_Linux.
 *
 * The calling convention is also dependant on the target binary desc (AMD64_Windows_Windows | AMD64_Linux_SysV)
 */
class TargetBinaryDesc
{
  public:
    ~TargetBinaryDesc() = default;

    /**
     * Returns true if the resulting binary needs to be encoded in little endian.
     */
    virtual bool isLittleEndian() const = 0;

    /**
     * Returns true if the code will be placed in a fixed position when running/linking. Look at PIC/PIE.
     *
     * If false, relocs are Absolute32 or Absolute64.
     * If true, the relocs can be of any of the other types.
     */
    virtual bool isPositionIndependent() const = 0;

    /**
     * Returns the name of the binary descriptor.
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the code model for this binary description.
     */
    virtual CodeModel getCodeModel() const = 0;

    /**
     * Returns the object format this ABI expects.
     */
    virtual ObjectFormat getObjectFormat() const = 0;

    /**
     * Returns the alignment needed for the starting address of a function. Imagine current address = 3. A new function
     * is going to be emitted, in Windows a function must start at a pointer-aligned address: 8.
     *
     * This makes the function's start address 8, the rest of the code (from 3, included, to 7, included) is filled with
     * PADDING (NOPS, INT3, ...).
     */
    virtual size_t getFunctionAlignment() const = 0;

    /**
     * Same as getFunctionAlignment but for loops.
     */
    virtual size_t getLoopAlignment() const = 0;

    /**
     * Returns the alignment needed for the given section name.
     */
    virtual size_t getSectionAlignment(std::string_view sectionName) const = 0;

  private:
};

#endif // EZPACKER_TARGETBINARYDESC_H