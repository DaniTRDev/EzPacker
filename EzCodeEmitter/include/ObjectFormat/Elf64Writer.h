#ifndef EZPACKER_ELF64_WRITER_H
#define EZPACKER_ELF64_WRITER_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"
#include "ObjectFormat/IObjectWriter.h"
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
class Elf64Writer : public IObjectWriter
{
  public:
    Elf64Writer() = default;
    ~Elf64Writer() override = default;

    /**
     * Adds an exported or internal symbol.
     */
    void addSymbol(const ObjectSymbol &sym) override;

    /**
     * Adds a relocation entry.
     */
    void addRelocation(const ObjectRelocEntry &reloc) override;

    /**
     * Generates a relocatable ELF64 object file byte stream from the provided code sections.
     */
    std::vector<uint8_t> write(const std::pmr::unordered_map<SectionType, CodeSection *> &sections) override;

    /**
     * Clears all registered symbols and relocations.
     */
    void clear() override;

  private:
    std::vector<ObjectSymbol> m_symbols;    ///< Symbols to materialize in .symtab.
    std::vector<ObjectRelocEntry> m_relocs; ///< Relocations to materialize in .rela.text.
};

} // namespace EzCodeEmitter::ObjectFormat

#endif // EZPACKER_ELF64_WRITER_H
