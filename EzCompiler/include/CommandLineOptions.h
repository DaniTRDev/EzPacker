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
    Object,        // Emits native binary object file (.o / .obj) [default]
    Assembly,      // Emits human-readable assembly text (.s)
    GenericMir,    // Dumps generic SSA MIR post-frontend lowering
    LegalizedMir,  // Dumps MIR post-type/opcode legalization
    LoweredMir     // Dumps target-lowered MIR post register allocation and frame lowering
};

/**
 * Optimization level determining pass pipeline configuration.
 */
enum class OptimizationLevel
{
    O0,
    O1,
    O2,
    Os
};

/**
 * Parsed configuration options governing the compilation run.
 */
struct CommandLineOptions
{
    std::string inputFilePath;
    std::string outputFilePath;
    TargetTriple target{ TargetTriple::getHostTriple() };
    EmissionStage emissionStage{ EmissionStage::Object };
    OptimizationLevel optLevel{ OptimizationLevel::O0 };

    bool verbose{ false };
    bool printPasses{ false };
    bool timePasses{ false };
    bool isPositionIndependent{ false };
    bool compileOnly{ true };
    bool color{ true };
    DiagnosticMessageType diagThreshold{ DiagnosticMessageType::Diag_Warning };
};

/**
 * Command-line argument parser utilizing argparse.
 */
class CommandLineParser
{
  public:
    CommandLineParser();

    bool parse(int argc, const char *const *argv, CommandLineOptions &outOptions, std::string &outError);
    bool parse(const std::vector<std::string> &args, CommandLineOptions &outOptions, std::string &outError);

    void printHelp() const;
    void printVersion() const;

  private:
    void setupArguments();

  private:
    std::unique_ptr<argparse::ArgumentParser> m_program;
};

} // namespace EzCompiler

#endif // EZPACKER_COMMAND_LINE_OPTIONS_H
