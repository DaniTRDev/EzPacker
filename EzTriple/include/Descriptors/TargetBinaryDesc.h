#ifndef EZTRIPLE_TARGET_BINARY_DESC_H
#define EZTRIPLE_TARGET_BINARY_DESC_H

#include "EzTripleCommon.h"

// Forward declaration
enum class SectionType : uint8_t;

/**
 * Code model controlling symbol address reachability and encoding size.
 */
enum class TargetCodeModel : uint8_t
{
    Small = 0, // Global addresses can be encoded in instruction pointer + small integer immediate.
    Large      // The entire address must be encoded in the instruction.
};

/**
 * Binary object container format targeting the host operating system.
 */
enum class TargetObjectFormat : uint8_t
{
    ELF = 0, ///< Executable and Linkable Format (System V, Linux/BSD).
    COFF,    ///< Common Object File Format (Windows).
    MachO    ///< Mach Object format (macOS/iOS).
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
    virtual ~TargetBinaryDesc() = default;

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
     * Returns the section of the given type, target must define EVERY section and overlap them if needed.
     */
    virtual class CodeSection *getSection(SectionType type) = 0;

    /**
     * Returns the code model for this binary description.
     */
    virtual TargetCodeModel getCodeModel() const = 0;

    /**
     * Returns the object format this ABI expects.
     */
    virtual TargetObjectFormat getObjectFormat() const = 0;

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
     * Initializes the binary descriptor and creates the needed structures.
     */
    virtual void initialize() = 0;

    /**
     * Returns the map of sections.
     */
    virtual const std::pmr::unordered_map<SectionType, class CodeSection *> &getSections() const = 0;

  private:
};

#endif // EZTRIPLE_TARGET_BINARY_DESC_H