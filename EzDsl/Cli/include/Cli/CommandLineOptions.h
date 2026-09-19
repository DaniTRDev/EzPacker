#ifndef EZDSL_CLI_COMMAND_LINE_OPTIONS_H
#define EZDSL_CLI_COMMAND_LINE_OPTIONS_H

#include "EzDslCliCommon.h"

namespace Cli
{

enum class GeneratorKind
{
    Auto,         // Inferred from file extension (.tyf -> TypeTable, .irdf -> Instructions, .lad -> Legalizer, .lrd -> Rules, .idf -> TargetInstructions, .isf -> InstructionSelector, .ezcc/.ccd -> CallingConv)
    TypeTable,    // CppMirTypeTableGenerator
    Instructions, // CppMirInstructionGenerator
    Legalizer,    // CppLegalizerGenerator
    Rules,        // CppLegalizeRuleGenerator
    TargetInstructions, // CppTargetInstructionGenerator
    TargetEncodings,    // CppTargetEncodingGenerator
    InstructionSelector, // CppInstructionSelectorGenerator
    CallingConv,  // CppCallingConvGenerator
    RegisterInfo, // CppRegisterInfoGenerator
    TargetDesc    // CppTargetDescGenerator
};

enum class OutputFormat
{
    Text,
    Json
};

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

struct CliOptions
{
    std::string inputFilePath;
    std::string outputPath{ "." };
    std::string targetName;
    std::vector<std::string> includeDirs;

    std::string rulesFilePath;        // --rules <file.lrd>
    std::string typesFilePath;        // --types <file.tyf>
    std::string instructionsFilePath; // --instructions <file.irdf>
    bool emitRules{ false };          // --emit-rules
    bool emitTargetInstructions{ false }; // --emit-target-instructions
    bool emitTargetEncodings{ false };    // --emit-target-encodings
    bool emitInstructionSelector{ false }; // --emit-instruction-selector
    bool emitCallingConv{ false };    // --emit-calling-conv
    bool emitRegisterInfo{ false };   // --emit-registers
    bool emitTargetDesc{ false };     // --emit-target-desc

    GeneratorKind generator{ GeneratorKind::Auto };
    LanguageDialect dialect{ LanguageDialect::Auto };

    bool headerOnly{ false };
    bool sourceOnly{ false };

    bool dumpInfo{ false };
    bool dumpAst{ false };
    bool dumpSymbols{ false };
    bool dumpFiles{ false };

    bool dryRun{ false };
    bool checkOnly{ false };
    bool verbose{ false };
    bool quiet{ false };

    OutputFormat format{ OutputFormat::Text };
};

class CommandLineParser
{
  public:
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
    void setupArguments();

  private:
    std::unique_ptr<argparse::ArgumentParser> m_program;
};

} // namespace Cli

#endif // EZDSL_CLI_COMMAND_LINE_OPTIONS_H
