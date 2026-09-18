#ifndef EZPACKER_ELF64_WRITER_H
#define EZPACKER_ELF64_WRITER_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"
#include "ObjectFormat/ObjectSymbol.h"
#include <vector>

namespace EzCodeEmitter::ObjectFormat
{

/**
 * Standard ELF64 Object File Writer (.o).
 * Emits complete, standard ELF64 relocatable object files compliant with
 * System V AMD64 ABI specification, containing valid ELF headers, section table,
 * string tables, symbol table, and relocation tables.
 */
class Elf64Writer
{
  public:
    Elf64Writer() = default;

    /**
     * Adds an exported or internal symbol.
     */
    void addSymbol(const ObjectSymbol &sym);

    /**
     * Adds a relocation entry.
     */
    void addRelocation(const ObjectRelocEntry &reloc);

    /**
     * Generates a relocatable ELF64 object file byte stream from the provided code sections.
     */
    std::vector<uint8_t> write(const std::pmr::unordered_map<SectionType, CodeSection *> &sections);

    /**
     * Clears all registered symbols and relocations.
     */
    void clear();

  private:
    std::vector<ObjectSymbol> m_symbols;
    std::vector<ObjectRelocEntry> m_relocs;
};

} // namespace EzCodeEmitter::ObjectFormat

#endif // EZPACKER_ELF64_WRITER_H
