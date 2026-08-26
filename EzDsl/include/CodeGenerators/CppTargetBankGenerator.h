#ifndef EZDSL_CPP_TARGET_BANK_GENERATOR_H
#define EZDSL_CPP_TARGET_BANK_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>
#include <string_view>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the register bank generation pass.
 */
enum class TargetBankGenWorkingMode : uint8_t
{
    Header = 1,            // Generates ONLY the header file (<Target>RegisterBanks.h).
    Source = 1 << 1,       // Generates ONLY the translation unit (<Target>RegisterBanks.cpp).
    Full = Header | Source // Generates both the header and the source files.
};

constexpr TargetBankGenWorkingMode operator|(TargetBankGenWorkingMode a, TargetBankGenWorkingMode b) noexcept
{
    return static_cast<TargetBankGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(TargetBankGenWorkingMode a, TargetBankGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes the EzMir Target Register and Bank model C++ class hierarchy and lookup logic from EzDSL .tdf metadata.
 *
 * ### Pipeline & Synthesized Structures:
 * 1. **Symbol Ingestion & Hierarchy Topological Sorting:**
 *    - Extracts `TargetSymbol`, `RegisterBankSymbol`, `RegisterClassSymbol`, and `RegisterSymbol` from `SymbolTable`.
 *    - Resolves transitive sub-register / super-register trees and sub-register bit offset alignments within root registers.
 *
 * 2. **Register Enumeration (`TargetReg`):**
 *    - Synthesizes strongly typed `enum class TargetReg : uint16_t` inside `namespace <TargetName>`.
 *    - Includes `NoRegister = 0`, all declared target hardware registers, and `TARGET_REG_COUNT`.
 *
 * 3. **Sub-Register Aliasing & Interference Matrix:**
 *    - Computes constant-time O(1) overlap bit table `TargetRegistersOverlap(regA, regB)` based on shared root register intervals.
 *    - Emits `GetSubRegisters(reg)` and `GetSuperRegisters(reg)` static tables.
 *
 * 4. **Register Bank & Class Model (`<TargetName>RegisterBanks`):**
 *    - Emits class `<TargetName>RegisterBanks` maintaining `MirRegisterBank`, `MirRegisterClass`, and `MirRegisterDescriptor` models.
 *    - Declares query and accessor functions (`getBank`, `getClass`, `getRegisterDescriptor`, `getRegisterRef`).
 *    - Implements factory function `CreateRegisterBanks(std::pmr::memory_resource *alloc)` for dynamic backend integration.
 *
 * 5. **Filesystem Output & Change Tracking:**
 *    - Synthesizes `<Target>RegisterBanks.h` and `<Target>RegisterBanks.cpp` (or explicitly specified path).
 *    - Uses timestamp-preserving `WriteFileIfChanged` to prevent unnecessary build recompilations.
 */
extern bool GenerateTargetRegisterBanks(class DiagnosticCollector *collector,
                                        class SymbolTable *table,
                                        std::filesystem::path outPath,
                                        std::string_view targetName = "",
                                        TargetBankGenWorkingMode mode = TargetBankGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_TARGET_BANK_GENERATOR_H
