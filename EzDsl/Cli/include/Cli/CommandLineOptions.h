#ifndef EZDSL_CLI_COMMAND_LINE_OPTIONS_H
#define EZDSL_CLI_COMMAND_LINE_OPTIONS_H

#include "EzDslCliCommon.h"

namespace Cli
{

/**
 * Identifies which backend code generator should be invoked for an input file.
 * Auto defers the choice to language/extension-based inference performed by the driver.
 */
enum class GeneratorKind
{
    Auto, // Inferred from file extension (.tyf -> TypeTable, .irdf -> Instructions, .lad -> Legalizer, .lrd -> Rules,
          // .idf -> TargetInstructions, .isf -> InstructionSelector, .ezcc/.ccd -> CallingConv)
    TypeTable,           // CppMirTypeTableGenerator
    Instructions,        // CppMirInstructionGenerator
    Legalizer,           // CppLegalizerGenerator
    Rules,               // CppLegalizeRuleGenerator
    TargetInstructions,  // CppTargetInstructionGenerator
    TargetEncodings,     // CppTargetEncodingGenerator
    InstructionSelector, // CppInstructionSelectorGenerator
    CallingConv,         // CppCallingConvGenerator
    RegisterInfo,        // CppRegisterInfoGenerator
    TargetDesc           // CppTargetDescGenerator
};

/** Selects the serialization used when dumping ASTs, symbols, or file metadata. */
enum class OutputFormat
{
    Text, ///< Human-readable, indented plain text.
    Json  ///< Machine-readable JSON.
};

/**
 * Identifies the EzDSL input language/dialect of a file, independent of its generator.
 * Auto defers detection to file-extension matching performed by the driver.
 */
enum class LanguageDialect
{
    Auto,
    TypeDef,           // .tyf
    IrInstDef,         // .irdf
    LegalizeAction,    // .lad
    LegalizeRule,      // .lrd
    TargetInstDef,     // .idf
    InstructionSelect, // .isf
    CallingConv,       // .ezcc, .ccd
    RegisterDef,       // .reg
    TargetDesc         // .tdesc
};

/**
 * Fully-resolved command line configuration consumed by the driver.
 * Fields are populated by CommandLineParser and represent requested inputs, outputs,
 * dump/emit toggles, and execution modes.
 */
struct CliOptions
{
    std::string inputFilePath;            ///< Primary input file to process.
    std::string outputPath{ "." };        ///< Destination file or directory for generated artifacts.
    std::string targetName;               ///< Target identifier substituted into generated code.
    std::vector<std::string> includeDirs; ///< Additional search paths for imported DSL files.

    std::string rulesFilePath;             // --rules <file.lrd>
    std::string typesFilePath;             // --types <file.tyf>
    std::string instructionsFilePath;      // --instructions <file.irdf>
    bool emitRules{ false };               // --emit-rules
    bool emitTargetInstructions{ false };  // --emit-target-instructions
    bool emitTargetEncodings{ false };     // --emit-target-encodings
    bool emitInstructionSelector{ false }; // --emit-instruction-selector
    bool emitCallingConv{ false };         // --emit-calling-conv
    bool emitRegisterInfo{ false };        // --emit-registers
    bool emitTargetDesc{ false };          // --emit-target-desc

    GeneratorKind generator{ GeneratorKind::Auto };   ///< Explicit generator override, or Auto to infer.
    LanguageDialect dialect{ LanguageDialect::Auto }; ///< Explicit dialect override, or Auto to infer.

    bool headerOnly{ false }; ///< Emit only the header artifact for paired generators.
    bool sourceOnly{ false }; ///< Emit only the source artifact for paired generators.

    bool dumpInfo{ false };    ///< Print general input/output file information.
    bool dumpAst{ false };     ///< Print the parsed AST.
    bool dumpSymbols{ false }; ///< Print the symbol table.
    bool dumpFiles{ false };   ///< Print the generated/expected file list.

    bool dryRun{ false };    ///< Report intended actions without writing any files.
    bool checkOnly{ false }; ///< Verify inputs and report diagnostics without generating output.
    bool verbose{ false };   ///< Enable verbose trace diagnostics.
    bool quiet{ false };     ///< Suppress non-error output.

    OutputFormat format{ OutputFormat::Text }; ///< Serialization format for dump output.
};

/**
 * Parses argv into a validated CliOptions and provides help/version text.
 */
class CommandLineParser
{
  public:
    /** Constructs the parser and registers all supported arguments and options. */
    CommandLineParser();

    /**
     * Parses command line arguments.
     * Returns std::nullopt if help/version was displayed or if parsing failed.
     * In case of parsing error, errorMessage will contain the failure reason.
     */
    std::optional<CliOptions> parse(int argc, char *argv[], std::string &errorMessage);

    /**
     * Returns the formatted help string.
     */
    std::string getHelp() const;

    /**
     * Returns the version string.
     */
    std::string getVersion() const;

  private:
    /** Registers the argument and option definitions with m_program. */
    void setupArguments();

  private:
    std::unique_ptr<argparse::ArgumentParser> m_program; ///< Underlying argparse program holding the option registry.
};

} // namespace Cli

#endif // EZDSL_CLI_COMMAND_LINE_OPTIONS_H
