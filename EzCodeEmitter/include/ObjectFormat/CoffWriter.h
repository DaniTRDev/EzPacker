#ifndef EZPACKER_COFF_WRITER_H
#define EZPACKER_COFF_WRITER_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"
#include "ObjectFormat/ObjectSymbol.h"
#include <vector>

namespace EzCodeEmitter::ObjectFormat
{

/**
 * Standard PE/COFF Relocatable Object File Writer (.obj).
 * Emits complete, standard AMD64 COFF object files compliant with
 * Microsoft Portable Executable and Common Object File Format Specification,
 * containing valid IMAGE_FILE_HEADER, IMAGE_SECTION_HEADERs, relocation tables,
 * COFF symbol table, and string table.
 */
class CoffWriter
{
  public:
    CoffWriter() = default;

    /**
     * Adds an exported or internal symbol.
     */
    void addSymbol(const ObjectSymbol &sym);

    /**
     * Adds a relocation entry.
     */
    void addRelocation(const ObjectRelocEntry &reloc);

    /**
     * Generates a relocatable PE/COFF object file byte stream from the provided code sections.
     */
    std::vector<uint8_t> write(const std::pmr::unordered_map<SectionType, CodeSection *> &sections);

    /**
     * Clears all registered symbols and relocations.
     */
    void clear();

  private:
    std::vector<ObjectSymbol> m_symbols;    ///< Symbols to materialize in the COFF symbol table.
    std::vector<ObjectRelocEntry> m_relocs; ///< Relocations to materialize per section.
};

} // namespace EzCodeEmitter::ObjectFormat

#endif // EZPACKER_COFF_WRITER_H
