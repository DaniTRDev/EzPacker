#ifndef EZPACKER_OBJECT_WRITER_H
#define EZPACKER_OBJECT_WRITER_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"
#include "ObjectFormat/ObjectSymbol.h"
#include <vector>

namespace EzCodeEmitter::ObjectFormat
{

/**
 * Format-independent interface for serializing sections, symbols and relocations into a
 * relocatable object file byte stream.
 *
 * Both the ELF64 and COFF writers implement this so the compiler's emission engine can select
 * the concrete writer once and feed it without duplicating the symbol/relocation plumbing.
 */
class IObjectWriter
{
  public:
    virtual ~IObjectWriter() = default;

    /**
     * Adds an exported or internal symbol.
     */
    virtual void addSymbol(const ObjectSymbol &sym) = 0;

    /**
     * Adds a relocation entry.
     */
    virtual void addRelocation(const ObjectRelocEntry &reloc) = 0;

    /**
     * Generates a relocatable object file byte stream from the provided code sections.
     */
    virtual std::vector<uint8_t> write(const std::pmr::unordered_map<SectionType, CodeSection *> &sections) = 0;

    /**
     * Clears all registered symbols and relocations.
     */
    virtual void clear() = 0;
};

} // namespace EzCodeEmitter::ObjectFormat

#endif // EZPACKER_OBJECT_WRITER_H
