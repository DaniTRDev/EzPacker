#ifndef EZPACKER_COMMAND_LINE_OPTIONS_H
#define EZPACKER_COMMAND_LINE_OPTIONS_H

#include "EzCompilerCommon.h"
#include "TargetTriple.h"
#include "Diagnostics/DiagnosticMessage.h"
#include <argparse/argparse.hpp>

namespace EzCompiler
{

/**
 * Compilation pipeline stopping gate determining what stage of output is emitted.
 */
enum class EmissionStage
{
    Object,       // Emits native binary object file (.o / .obj) [default]
    Assembly,     // Emits human-readable assembly text (.s)
    GenericMir,   // Dumps generic SSA MIR post-frontend lowering
    LegalizedMir, // Dumps MIR post-type/opcode legalization
    LoweredMir    // Dumps target-lowered MIR post register allocation and frame lowering
};

/**
 * Optimization level determining pass pipeline configuration.
 */
enum class OptimizationLevel
{
    O0, ///< No optimization.
    O1, ///< Basic optimizations.
    O2, ///< Full optimization pipeline.
    Os  ///< Optimize primarily for code size.
};

/**
 * Parsed configuration options governing the compilation run.
 */
struct CommandLineOptions
{
    std::string inputFilePath;                            ///< Source/IR file to compile.
    std::string outputFilePath;                           ///< Destination file path.
    TargetTriple target{ TargetTriple::getHostTriple() }; ///< Requested target triple.
    EmissionStage emissionStage{ EmissionStage::Object }; ///< Stage at which compilation stops.
    OptimizationLevel optLevel{ OptimizationLevel::O0 };  ///< Requested optimization level.

    bool verbose{ false };               ///< Enables verbose diagnostic logging.
    bool printPasses{ false };           ///< Prints pass names as they run.
    bool timePasses{ false };            ///< Reports per-pass execution times.
    bool isPositionIndependent{ false }; ///< Generates position-independent code.
    bool compileOnly{ true };            ///< Compile/assemble without linking.
    bool color{ true };                  ///< Enables colored diagnostic output.
    DiagnosticMessageType diagThreshold{ DiagnosticMessageType::Diag_Warning }; ///< Minimum reported severity.
};

/**
 * Command-line argument parser utilizing argparse.
 */
class CommandLineParser
{
  public:
    /**
     * Constructs the parser and registers all supported command-line arguments.
     */
    CommandLineParser();

    /**
     * Parses argv and populates outOptions. On failure, outError receives the reason.
     * Returns false when parsing fails or when a version/help request short-circuits.
     */
    bool parse(int argc, const char *const *argv, CommandLineOptions &outOptions, std::string &outError);

    /**
     * Parses a pre-tokenized argument list and populates outOptions (see the argv overload).
     */
    bool parse(const std::vector<std::string> &args, CommandLineOptions &outOptions, std::string &outError);

    /**
     * Prints the argparse-generated usage/help text.
     */
    void printHelp() const;

    /**
     * Prints the compiler version banner.
     */
    void printVersion() const;

  private:
    /**
     * Builds the argparse program definition, declaring every accepted flag.
     */
    void setupArguments();

  private:
    std::unique_ptr<argparse::ArgumentParser> m_program; ///< Owned argparse parser instance.
};

} // namespace EzCompiler

#endif // EZPACKER_COMMAND_LINE_OPTIONS_H
