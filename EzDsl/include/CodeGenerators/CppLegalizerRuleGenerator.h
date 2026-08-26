#ifndef EZDSL_CPP_LEGALIZER_RULE_GENERATOR_H
#define EZDSL_CPP_LEGALIZER_RULE_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>
#include <string_view>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the legalize rule engine generation pass.
 */
enum class LegalizerRuleGenWorkingMode : uint8_t
{
    Header = 1,             // Generates ONLY the legalize rules header (<Target>LegalizeRules.h).
    Source = 1 << 1,        // Generates ONLY the legalize rules translation unit (<Target>LegalizeRules.cpp).
    Full = Header | Source  // Generates both header and source files.
};

constexpr LegalizerRuleGenWorkingMode operator|(LegalizerRuleGenWorkingMode a,
                                                LegalizerRuleGenWorkingMode b) noexcept
{
    return static_cast<LegalizerRuleGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(LegalizerRuleGenWorkingMode a, LegalizerRuleGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes target legalization rewrite rules and AST expansion engine from EzDSL .lrd definitions.
 */
extern bool GenerateTargetLegalizerRules(class DiagnosticCollector *collector,
                                         class SymbolTable *table,
                                         std::filesystem::path outPath,
                                         std::string_view targetName = "",
                                         LegalizerRuleGenWorkingMode mode = LegalizerRuleGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_LEGALIZER_RULE_GENERATOR_H
