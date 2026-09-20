#ifndef EZDSL_CLI_DRIVER_H
#define EZDSL_CLI_DRIVER_H

#include "Cli/CommandLineOptions.h"
#include "Cli/InfoDumper.h"
#include "EzDslCliCommon.h"

namespace Cli
{

/**
 * Outcome of a Driver::run() invocation, including any requested file lists and failure detail.
 */
struct DriverResult
{
    bool success{ false };                      ///< True when all requested processing completed without error.
    std::string errorMessage;                   ///< Human-readable failure reason when success is false.
    std::vector<OutputFileInfo> generatedFiles; ///< Files actually written or that would be written in a dry run.
    std::vector<OutputFileInfo> unchangedFiles; ///< Expected outputs skipped because their content was unchanged.
};

/**
 * Orchestrates a single CLI invocation: detects the dialect, selects a generator,
 * invokes the appropriate code-generation entry point, and optionally dumps info/AST/symbols.
 */
class Driver
{
  public:
    /** Captures the resolved CLI options for subsequent run() execution. */
    explicit Driver(CliOptions options);

    /** Executes the requested pipeline and returns its result. */
    DriverResult run();

  private:
    /** Maps a file's extension to a LanguageDialect, returning Auto when the extension is unrecognized. */
    LanguageDialect detectDialect(const std::filesystem::path &filePath) const;

    /** Maps a detected dialect to the code generator that consumes it. */
    GeneratorKind resolveGeneratorKind(LanguageDialect dialect) const;

    /** Resolves the target identifier (explicit, input stem, or fallback) sanitized for C++ identifiers. */
    std::string resolveTargetName(std::string_view fallback) const;

    /** Computes the output files a generator is expected to produce under outDir, for reporting and dry runs. */
    std::vector<OutputFileInfo> computeExpectedOutputs(GeneratorKind genKind,
                                                       const std::filesystem::path &outDir) const;

  private:
    CliOptions m_options; ///< Resolved options driving this invocation.
};

} // namespace Cli

#endif // EZDSL_CLI_DRIVER_H
