#ifndef EZDSL_CPP_ISEL_TABLE_GENERATOR_H
#define EZDSL_CPP_ISEL_TABLE_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>
#include <string_view>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the instruction selection table generation pass.
 */
enum class ISelTableGenWorkingMode : uint8_t
{
    Header = 1,             // Generates ONLY the instruction selector header (<Target>ISelTable.h).
    Source = 1 << 1,        // Generates ONLY the instruction selector translation unit (<Target>ISelTable.cpp).
    Full = Header | Source  // Generates both header and source files.
};

constexpr ISelTableGenWorkingMode operator|(ISelTableGenWorkingMode a, ISelTableGenWorkingMode b) noexcept
{
    return static_cast<ISelTableGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(ISelTableGenWorkingMode a, ISelTableGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes target instruction selection decision trees and addressing mode matchers
 * from EzDSL .isf definition metadata.
 */
extern bool GenerateTargetISelTable(class DiagnosticCollector *collector,
                                    class SymbolTable *table,
                                    std::filesystem::path outPath,
                                    std::string_view targetName = "",
                                    ISelTableGenWorkingMode mode = ISelTableGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_ISEL_TABLE_GENERATOR_H
