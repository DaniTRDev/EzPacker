#ifndef EZDSL_CPP_MIR_TYPE_TABLE_GENERATOR_H
#define EZDSL_CPP_MIR_TYPE_TABLE_GENERATOR_H

#include "EzDslCommon.h"

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the code generation pass.
 */
enum class MirTypeTableGenWorkingMode : uint8_t
{
    Header = 1,            // Generates ONLY the header file (MirTypeTable.h).
    Source = 1 << 1,       // Generates ONLY the translation unit (MirTypeTable.cpp).
    Full = Header | Source // Generates both the header and the source files.
};

constexpr MirTypeTableGenWorkingMode operator|(MirTypeTableGenWorkingMode a, MirTypeTableGenWorkingMode b) noexcept
{
    return static_cast<MirTypeTableGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(MirTypeTableGenWorkingMode a, MirTypeTableGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes the EzMir TypeTable C++ class hierarchy and initialization logic from EzDSL metadata.
 *
 * ### Pipeline & How It Works:
 * 1. **Symbol Ingestion & Filtering:**
 *    - Traverses `table->getSymbols()` to filter out all symbols where `Symbol::getType() == SymbolType::Type`.
 *    - Extracts `TypeData` (`m_bitWidth`, `m_name`) from the symbol's variant storage.
 *
 * 2. **Kind Deduction & Signature Synthesis:**
 *    - Categorizes each type into its corresponding `MirTypeKind` (`Integer`, `FloatingPoint`, `Void`, or
 * `BindingToken`) based on identifier prefixes (e.g., 'i' -> integer, 'f' -> float).
 *    - Synthesizes accessor method names (e.g., `MirType *i32()`) and member pointer fields (`MirType *m_i32Type`).
 *
 * 3. **Header Generation (`.h`):**
 *    - Emits class definition `MirTypeTable` maintaining arena compatibility (`std::pmr::memory_resource*`).
 *    - Declares dynamic type creation APIs (`create`, `getClass`, `getFuncType`, `getPtr`, `getArray`).
 *    - Emits direct inline accessors for each type symbol discovered in the `.tyf` files.
 *    - Declares tracking data structures: `m_typeNames` (hash name interning), `m_idToType` (dense ID lookup),
 *      and `m_pointerCache` (canonical pointer deduplication).
 *
 * 4. **Source Generation (`.cpp`):**
 *    - Emits target alignment and size calculation logic mediated by `IMirTargetTypeLayout`.
 *    - Implements structural type interning (guaranteeing pointer identity for identical composite types).
 *    - Implements `MirTypeTable::initialize(IMirTargetTypeLayout *typeLayout)` which sequentially constructs
 *      and interns all DSL-declared types in the arena.
 *
 * 5. **Filesystem Resolution & Output:**
 *    - Inspects `outPath`: if a directory is given, generates `MirTypeTable.h` and `MirTypeTable.cpp` inside it.
 *      If a file path with an extension is provided, adjusts extensions accordingly.
 *    - Emits diagnostics via `DiagnosticCollector` for tracing, file I/O errors, or malformed symbols.
 */
extern bool GenerateMirTypeTable(class DiagnosticCollector *collector,
                                 class SymbolTable *table,
                                 std::filesystem::path outPath,
                                 MirTypeTableGenWorkingMode mode = MirTypeTableGenWorkingMode::Full);

}; // namespace CodeGenerators

#endif // EZDSL_CPP_MIR_TYPE_TABLE_GENERATOR_H