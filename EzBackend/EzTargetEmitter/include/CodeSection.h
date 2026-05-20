#ifndef EZPACKER_CODESECTION_H
#define EZPACKER_CODESECTION_H

#include "EzTargetEmitterCommon.h"

enum class CodeSectionFlags
{
    Executable = (1 << 0), // .text
    Readable = (1 << 1),   // .rodata, .text, .data
    Writable = (1 << 2)    // .data, .bss
};

struct CodeRelocation
{
    const char *m_symbolName; // Name of the symbol to which this relocation refers
    uint64_t m_offset;        // Offset within the section
    uint64_t m_symbolIndex;   // Index of the symbol in the symbol table
    uint32_t m_type;          // Type of relocation (e.g., R_X86_64_PC32)
};

class CodeSection
{
  public:
    /**
     * Creates a section with the given information.
     * @param name
     * @param flags
     * @param data
     * @param relocations
     */
    CodeSection(const char *name,
                CodeSectionFlags flags,
                ConstantArray<uint8_t> *data,
                TypedPoolLinkedList<CodeRelocation> *relocations);

    /**
     * Adds a relocation in this section.
     * @param reloc
     * @return
     */
    bool addReloc(const CodeRelocation &reloc);

    /**
     * Returns the name of the section.
     * @return
     */
    const char *getName() const;

    /**
     * Searches for a relocation that applies to the given address in the current code section. If found, returns a
     * pointer to the CodeRelocation; otherwise, returns nullptr.
     * @param address
     * @return
     */
    CodeRelocation *getRelocationAtAddr(uint64_t address) const;

    /**
     * Returns the flags of the section.
     * @return
     */
    CodeSectionFlags getFlags() const;

    /**
     * Returns the data contained in this section.
     * @return
     */
    ConstantArray<uint8_t> *getData() const;

    /**
     * Returns the list of relocations.
     * @return
     */
    TypedPoolLinkedList<CodeRelocation> *getRelocations() const;

  private:
    const char *m_name;
    CodeSectionFlags m_flags;
    ConstantArray<uint8_t> *m_data;
    TypedPoolLinkedList<CodeRelocation> *m_relocations;
};

#endif // EZPACKER_CODESECTION_H
