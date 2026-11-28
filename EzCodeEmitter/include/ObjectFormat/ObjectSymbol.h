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

/**
 * Relocation entry targeting a symbol or section from an extra (e.g. debug) object section.
 */
struct ExtraSectionReloc
{
    uint64_t m_offset{ 0 };                                                ///< Byte offset of the relocation field within the extra section.
    std::string m_targetName;                                              ///< Target symbol or section name.
    bool m_targetIsSection{ false };                                       ///< True if m_targetName names a section rather than a symbol.
    TargetCodeRelocationType m_type{ TargetCodeRelocationType::Absolute64 }; ///< Fixup semantics.
    int64_t m_addend{ 0 };                                                 ///< Constant added to the resolved target value.
};

/**
 * Raw extra section (e.g., DWARF or CodeView debug section) to be emitted alongside standard code/data sections.
 */
struct ExtraObjectSection
{
    std::string m_name;                      ///< Section name (e.g., ".debug_info", ".debug$S").
    std::vector<uint8_t> m_data;             ///< Raw section payload bytes.
    uint64_t m_alignment{ 1 };               ///< Required section alignment in bytes.
    bool m_isDebug{ true };                  ///< True for non-allocated debug sections.
    bool m_isAlloc{ false };                 ///< True if section should be allocated in memory (SHF_ALLOC).
    bool m_isMergeStrings{ false };          ///< True for mergeable null-terminated string sections (SHF_MERGE | SHF_STRINGS).
    std::vector<ExtraSectionReloc> m_relocs; ///< Relocations applied to this section's data.
};

} // namespace EzCodeEmitter::ObjectFormat

#endif // EZPACKER_OBJECT_SYMBOL_H
