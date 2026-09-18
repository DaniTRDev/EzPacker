#ifndef EZPACKER_OBJECT_SYMBOL_H
#define EZPACKER_OBJECT_SYMBOL_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"
#include "CodeEmitterContext.h"
#include <string>

namespace EzCodeEmitter::ObjectFormat
{

/**
 * Symbol descriptor for object file emission (ELF64 / PE-COFF).
 */
struct ObjectSymbol
{
    std::string m_name;
    SectionType m_section{ SectionType::Text };
    uint64_t m_offset{ 0 };
    uint64_t m_size{ 0 };
    bool m_isGlobal{ true };
    bool m_isFunction{ false };
};

/**
 * Relocation entry for object file emission.
 */
struct ObjectRelocEntry
{
    SectionType m_section{ SectionType::Text };
    uint64_t m_offset{ 0 };
    std::string m_symbolName;
    TargetCodeRelocationType m_type{ TargetCodeRelocationType::None };
    int64_t m_addend{ 0 };
};

} // namespace EzCodeEmitter::ObjectFormat

#endif // EZPACKER_OBJECT_SYMBOL_H
