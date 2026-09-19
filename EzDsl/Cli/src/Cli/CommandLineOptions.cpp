#include "Cli/CommandLineOptions.h"

namespace Cli
{

CommandLineParser::CommandLineParser()
{
    setupArguments();
}

void CommandLineParser::setupArguments()
{
    m_program = std::make_unique<argparse::ArgumentParser>("EzDslCli", "1.0.0", argparse::default_arguments::help);

    m_program->add_description("EzDSL Compiler Backend Driver & Code Generator Tool");

    m_program->add_argument("-i", "--input")
            .help("Path to the input EzDSL definition file (.tyf, .irdf, .lad, .lrd)")
            .metavar("<file>")
            .default_value(std::string(""));

    m_program->add_argument("-o", "--output")
            .help("Output directory or destination file path (default: .)")
            .metavar("<path>")
            .default_value(std::string("."));

    m_program->add_argument("-I", "--include")
            .help("Directory to search for included files (can be specified multiple times)")
            .metavar("<dir>")
            .append();

    m_program->add_argument("--emit-type-table")
            .help("Synthesize EzMir MirTypeTable (.h and/or .cpp)")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--emit-instructions")
            .help("Synthesize EzMir MirInstructionSetDefs (.h)")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--emit-legalizer")
            .help("Synthesize Target LegalizerActionTable (.h and .cpp)")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--emit-rules")
            .help("Synthesize Target LegalizerRules (.h and .cpp) from .lrd")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--emit-target-instructions")
            .help("Synthesize Target TargetInstructionTable (.h and .cpp) from .idf")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--emit-instruction-selector")
            .help("Synthesize Target InstructionSelector (.h and .cpp) from .isf")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--emit-calling-conv")
            .help("Synthesize Target CallingConvDesc (.h and .cpp) from .ezcc / .ccd")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--emit-registers")
            .help("Synthesize Target RegisterInfo (.h) from .reg")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--emit-target-desc")
            .help("Synthesize TargetDesc (.h and .cpp) from .tdesc")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--rules")
            .help("Path to companion .lrd rewrite rules file")
            .metavar("<file>")
            .default_value(std::string(""));

    m_program->add_argument("--types")
            .help("Path to dependency .tyf type definition file")
            .metavar("<file>")
            .default_value(std::string(""));

    m_program->add_argument("--instructions")
            .help("Path to dependency .irdf instruction definition file")
            .metavar("<file>")
            .default_value(std::string(""));

    m_program->add_argument("--target")
            .help("Target architecture name for code generation (e.g. AMD64, AArch64)")
            .metavar("<target>")
            .default_value(std::string(""));

    m_program->add_argument("--generator")
            .help("Explicit generator to execute: 'type-table', 'instructions', 'legalizer', or 'auto'")
            .metavar("<gen>")
            .default_value(std::string("auto"));

    m_program->add_argument("--header-only")
            .help("Synthesize only header (.h) file")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--source-only")
            .help("Synthesize only translation unit (.cpp) file")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--dump-info")
            .help("Dump file metadata, recognized dialect, and construct counts")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--dump-ast")
            .help("Dump the parsed AST structure")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--dump-symbols")
            .help("Dump the populated symbol table after semantic analysis")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--dump-files")
            .help("Dump the list of expected/generated output files")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--dry-run")
            .help("Parse, validate, and compute outputs without writing any files to disk")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--check-only")
            .help("Only perform syntactic and semantic validation passes")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("-v", "--verbose")
            .help("Enable verbose diagnostic trace and debug output")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("-q", "--quiet")
            .help("Suppress non-essential console output")
            .default_value(false)
            .implicit_value(true);

    m_program->add_argument("--format")
            .help("Output format for dump operations: 'text' or 'json' (default: text)")
            .metavar("<fmt>")
            .default_value(std::string("text"));

    m_program->add_argument("--version")
            .help("Display tool version and exit")
            .default_value(false)
            .implicit_value(true);
}

std::optional<CliOptions> CommandLineParser::parse(int argc, char *argv[], std::string &errorMessage)
{
    try
    {
        m_program->parse_args(argc, argv);
    }
    catch (const std::exception &err)
    {
        errorMessage = err.what();
        return std::nullopt;
    }

    if (m_program->get<bool>("--version"))
    {
        std::cout << getVersion() << "\n";
        return std::nullopt;
    }

    CliOptions opts;

    opts.inputFilePath = m_program->get<std::string>("-i");
    if (opts.inputFilePath.empty())
    {
        errorMessage = "Missing required input file (-i, --input <file>).";
        return std::nullopt;
    }

    opts.outputPath = m_program->get<std::string>("-o");

    if (m_program->is_used("-I"))
    {
        opts.includeDirs = m_program->get<std::vector<std::string>>("-I");
    }

    opts.headerOnly = m_program->get<bool>("--header-only");
    opts.sourceOnly = m_program->get<bool>("--source-only");
    opts.dumpInfo = m_program->get<bool>("--dump-info");
    opts.dumpAst = m_program->get<bool>("--dump-ast");
    opts.dumpSymbols = m_program->get<bool>("--dump-symbols");
    opts.dumpFiles = m_program->get<bool>("--dump-files");
    opts.dryRun = m_program->get<bool>("--dry-run");
    opts.checkOnly = m_program->get<bool>("--check-only");
    opts.verbose = m_program->get<bool>("-v");
    opts.quiet = m_program->get<bool>("-q");

    std::string formatStr = m_program->get<std::string>("--format");
    std::transform(formatStr.begin(), formatStr.end(), formatStr.begin(), [](unsigned char c) { return std::tolower(c); });
    if (formatStr == "json")
    {
        opts.format = OutputFormat::Json;
    }
    else
    {
        opts.format = OutputFormat::Text;
    }

    opts.targetName = m_program->get<std::string>("--target");

    opts.rulesFilePath = m_program->get<std::string>("--rules");
    opts.typesFilePath = m_program->get<std::string>("--types");
    opts.instructionsFilePath = m_program->get<std::string>("--instructions");
    opts.emitRules = m_program->get<bool>("--emit-rules");
    opts.emitTargetInstructions = m_program->get<bool>("--emit-target-instructions");
    opts.emitInstructionSelector = m_program->get<bool>("--emit-instruction-selector");
    opts.emitCallingConv = m_program->get<bool>("--emit-calling-conv");
    opts.emitRegisterInfo = m_program->get<bool>("--emit-registers");
    opts.emitTargetDesc = m_program->get<bool>("--emit-target-desc");

    bool emitTypeTable = m_program->get<bool>("--emit-type-table");
    bool emitInstructions = m_program->get<bool>("--emit-instructions");
    bool emitLegalizer = m_program->get<bool>("--emit-legalizer");
    bool emitRules = opts.emitRules;
    bool emitTargetInstructions = opts.emitTargetInstructions;
    bool emitInstructionSelector = opts.emitInstructionSelector;
    bool emitCallingConv = opts.emitCallingConv;
    bool emitRegisterInfo = opts.emitRegisterInfo;
    bool emitTargetDesc = opts.emitTargetDesc;
    std::string explicitGen = m_program->get<std::string>("--generator");
    std::transform(explicitGen.begin(), explicitGen.end(), explicitGen.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (emitTypeTable && emitInstructions)
    {
        errorMessage = "Cannot specify both --emit-type-table and --emit-instructions simultaneously.";
        return std::nullopt;
    }

    size_t emitCount = (emitTypeTable ? 1 : 0) + (emitInstructions ? 1 : 0) + (emitLegalizer ? 1 : 0) + (emitRules ? 1 : 0)
                     + (emitTargetInstructions ? 1 : 0) + (emitInstructionSelector ? 1 : 0) + (emitCallingConv ? 1 : 0)
                     + (emitRegisterInfo ? 1 : 0) + (emitTargetDesc ? 1 : 0);
    if (emitCount > 1)
    {
        errorMessage = "Cannot specify multiple generator emission flags simultaneously.";
        return std::nullopt;
    }

    if (emitTypeTable)
    {
        opts.generator = GeneratorKind::TypeTable;
    }
    else if (emitInstructions)
    {
        opts.generator = GeneratorKind::Instructions;
    }
    else if (emitLegalizer)
    {
        opts.generator = GeneratorKind::Legalizer;
    }
    else if (emitRules)
    {
        opts.generator = GeneratorKind::Rules;
    }
    else if (emitTargetInstructions)
    {
        opts.generator = GeneratorKind::TargetInstructions;
    }
    else if (emitInstructionSelector)
    {
        opts.generator = GeneratorKind::InstructionSelector;
    }
    else if (emitCallingConv)
    {
        opts.generator = GeneratorKind::CallingConv;
    }
    else if (emitRegisterInfo)
    {
        opts.generator = GeneratorKind::RegisterInfo;
    }
    else if (emitTargetDesc)
    {
        opts.generator = GeneratorKind::TargetDesc;
    }
    else if (explicitGen == "type-table" || explicitGen == "typetable")
    {
        opts.generator = GeneratorKind::TypeTable;
    }
    else if (explicitGen == "instructions" || explicitGen == "instruction")
    {
        opts.generator = GeneratorKind::Instructions;
    }
    else if (explicitGen == "legalizer" || explicitGen == "legalize")
    {
        opts.generator = GeneratorKind::Legalizer;
    }
    else if (explicitGen == "rules" || explicitGen == "rule")
    {
        opts.generator = GeneratorKind::Rules;
    }
    else if (explicitGen == "target-instructions" || explicitGen == "target_instructions" || explicitGen == "target-inst")
    {
        opts.generator = GeneratorKind::TargetInstructions;
    }
    else if (explicitGen == "instruction-selector" || explicitGen == "instruction_selector" || explicitGen == "isel")
    {
        opts.generator = GeneratorKind::InstructionSelector;
    }
    else if (explicitGen == "calling-conv" || explicitGen == "calling_conv" || explicitGen == "callingconv" || explicitGen == "cc")
    {
        opts.generator = GeneratorKind::CallingConv;
    }
    else if (explicitGen == "registers" || explicitGen == "register" || explicitGen == "register-info" || explicitGen == "reg")
    {
        opts.generator = GeneratorKind::RegisterInfo;
    }
    else if (explicitGen == "target-desc" || explicitGen == "target_desc" || explicitGen == "targetdesc" || explicitGen == "tdesc")
    {
        opts.generator = GeneratorKind::TargetDesc;
    }
    else
    {
        opts.generator = GeneratorKind::Auto;
    }

    return opts;
}

std::string CommandLineParser::getHelp() const
{
    if (!m_program)
    {
        return {};
    }
    return m_program->help().str();
}

std::string CommandLineParser::getVersion() const
{
    return "EzDslCli version 1.0.0 (EzPacker Compiler Suite)";
}

} // namespace Cli
