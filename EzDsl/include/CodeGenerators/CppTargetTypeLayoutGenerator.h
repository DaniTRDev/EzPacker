#ifndef EZDSL_CPP_TARGET_TYPE_LAYOUT_GENERATOR_H
#define EZDSL_CPP_TARGET_TYPE_LAYOUT_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>
#include <string_view>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the target type layout generation pass.
 */
enum class TargetTypeLayoutGenWorkingMode : uint8_t
{
    Header = 1,             // Generates ONLY the type layout header (<Target>TypeLayout.h).
    Source = 1 << 1,        // Generates ONLY the type layout translation unit (<Target>TypeLayout.cpp).
    Full = Header | Source  // Generates both header and source files.
};

constexpr TargetTypeLayoutGenWorkingMode operator|(TargetTypeLayoutGenWorkingMode a,
                                                   TargetTypeLayoutGenWorkingMode b) noexcept
{
    return static_cast<TargetTypeLayoutGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(TargetTypeLayoutGenWorkingMode a, TargetTypeLayoutGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes the target machine type layout class implementing IMirTargetTypeLayout
 * from EzDSL type definitions (.tyf) and target architecture pointer configuration.
 */
extern bool GenerateTargetTypeLayout(class DiagnosticCollector *collector,
                                     class SymbolTable *table,
                                     std::filesystem::path outPath,
                                     std::string_view targetName = "",
                                     TargetTypeLayoutGenWorkingMode mode = TargetTypeLayoutGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_TARGET_TYPE_LAYOUT_GENERATOR_H
