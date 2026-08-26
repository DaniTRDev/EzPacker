#ifndef EZDSL_CPP_TARGET_DESC_GENERATOR_H
#define EZDSL_CPP_TARGET_DESC_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>
#include <string_view>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the target descriptor generation pass.
 */
enum class TargetDescGenWorkingMode : uint8_t
{
    Header = 1,             // Generates ONLY the target descriptor header (<Target>TargetDesc.h).
    Source = 1 << 1,        // Generates ONLY the target descriptor translation unit (<Target>TargetDesc.cpp).
    Full = Header | Source  // Generates both header and source files.
};

constexpr TargetDescGenWorkingMode operator|(TargetDescGenWorkingMode a, TargetDescGenWorkingMode b) noexcept
{
    return static_cast<TargetDescGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(TargetDescGenWorkingMode a, TargetDescGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes the master TargetDesc and TargetBinaryDesc target backend plugin class
 * tying together register banks, type layout, instruction sets, and calling conventions.
 */
extern bool GenerateTargetDescriptor(class DiagnosticCollector *collector,
                                     class SymbolTable *table,
                                     std::filesystem::path outPath,
                                     std::string_view targetName = "",
                                     TargetDescGenWorkingMode mode = TargetDescGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_TARGET_DESC_GENERATOR_H
