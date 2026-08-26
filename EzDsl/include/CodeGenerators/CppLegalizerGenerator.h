#ifndef EZDSL_CPP_LEGALIZER_GENERATOR_H
#define EZDSL_CPP_LEGALIZER_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>
#include <string_view>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the legalizer action matrix generation pass.
 */
enum class LegalizerGenWorkingMode : uint8_t
{
    Header = 1,             // Generates ONLY the legalizer header (<Target>LegalizerActionTable.h).
    Source = 1 << 1,        // Generates ONLY the legalizer source (<Target>LegalizerActionTable.cpp).
    Full = Header | Source  // Generates both header and source files.
};

constexpr LegalizerGenWorkingMode operator|(LegalizerGenWorkingMode a, LegalizerGenWorkingMode b) noexcept
{
    return static_cast<LegalizerGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(LegalizerGenWorkingMode a, LegalizerGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes the 2D constant-time legality action matrix from EzDSL .lad definition metadata.
 */
extern bool GenerateTargetLegalizerTable(class DiagnosticCollector *collector,
                                         class SymbolTable *table,
                                         std::filesystem::path outPath,
                                         std::string_view targetName = "",
                                         LegalizerGenWorkingMode mode = LegalizerGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_LEGALIZER_GENERATOR_H
