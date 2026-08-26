#ifndef EZDSL_CPP_CALLING_CONV_GENERATOR_H
#define EZDSL_CPP_CALLING_CONV_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>
#include <string_view>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the calling convention generation pass.
 */
enum class CallingConvGenWorkingMode : uint8_t
{
    Header = 1,             // Generates ONLY the calling convention header (<Target>CallingConventions.h).
    Source = 1 << 1,        // Generates ONLY the calling convention translation unit (<Target>CallingConventions.cpp).
    Full = Header | Source  // Generates both header and source files.
};

constexpr CallingConvGenWorkingMode operator|(CallingConvGenWorkingMode a,
                                              CallingConvGenWorkingMode b) noexcept
{
    return static_cast<CallingConvGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(CallingConvGenWorkingMode a, CallingConvGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes target Calling Convention descriptor classes and factories
 * from EzDSL .ccdf definition metadata.
 */
extern bool GenerateTargetCallingConventions(class DiagnosticCollector *collector,
                                             class SymbolTable *table,
                                             std::filesystem::path outPath,
                                             std::string_view targetName = "",
                                             CallingConvGenWorkingMode mode = CallingConvGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_CALLING_CONV_GENERATOR_H
