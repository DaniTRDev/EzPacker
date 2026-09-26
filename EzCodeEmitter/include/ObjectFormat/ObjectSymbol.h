#ifndef EZPACKER_OBJECT_SYMBOL_H
#define EZPACKER_OBJECT_SYMBOL_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"
#include "CodeEmitterContext.h"
#include <string_view>

namespace EzCodeEmitter::ObjectFormat
{

/**
 * Symbol descriptor for object file emission (ELF64 / PE-COFF).
 *
 * The name is a non-owning view so the emission path never re-allocates a string per symbol. The
 * referenced storage must outlive both the descriptor and any object writer it is handed to;
 * callers pass views into MIR-context-owned names (MirFunction/MirGlobalVar) or string literals.
 */
struct ObjectSymbol
{
    std::string_view m_name;                    ///< Symbol name as it appears in the object file.
    SectionType m_section{ SectionType::Text }; ///< Section the symbol is defined in.
    uint64_t m_offset{ 0 };                     ///< Byte offset from the start of its section.
    uint64_t m_size{ 0 };                       ///< Size of the symbol's data in bytes.
    bool m_isGlobal{ true };                    ///< True for external linkage, false for local.
    bool m_isWeak{ false };                     ///< True for weak linkage.
    bool m_isFunction{ false };                 ///< True when the symbol marks executable code.
};

/**
 * Relocation entry for object file emission.
 *
 * m_symbolName follows the same non-owning lifetime contract as ObjectSymbol::m_name.
 */
struct ObjectRelocEntry
{
    SectionType m_section{ SectionType::Text }; ///< Section containing the field to be patched.
    uint64_t m_offset{ 0 };                     ///< Byte offset of the relocation field in that section.
    std::string_view m_symbolName;              ///< Name of the symbol the relocation points at.
    TargetCodeRelocationType m_type{ TargetCodeRelocationType::None }; ///< Fixup semantics.
    int64_t m_addend{ 0 };                                             ///< Constant added to the resolved symbol value.
};

} // namespace EzCodeEmitter::ObjectFormat

#endif // EZPACKER_OBJECT_SYMBOL_H
