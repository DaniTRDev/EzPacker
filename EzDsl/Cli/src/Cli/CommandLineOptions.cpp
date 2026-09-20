#include "Cli/CommandLineOptions.h"

namespace Cli
{

namespace
{

// One --emit-* switch: its flag spelling, optional CliOptions mirror field, and generator it selects.
struct EmitOption
{
    const char *m_flag;           ///< argparse option name.
    bool CliOptions::*m_mirror;   ///< CliOptions field kept in sync, or nullptr when not mirrored.
    GeneratorKind m_generator;    ///< Generator selected when the flag is present.
};

// Emission switches in precedence order; a new generator is added here once.
constexpr EmitOption kEmitOptions[] = {
    { "--emit-type-table", nullptr, GeneratorKind::TypeTable },
    { "--emit-instructions", nullptr, GeneratorKind::Instructions },
    { "--emit-legalizer", nullptr, GeneratorKind::Legalizer },
    { "--emit-rules", &CliOptions::emitRules, GeneratorKind::Rules },
    { "--emit-target-instructions", &CliOptions::emitTargetInstructions, GeneratorKind::TargetInstructions },
    { "--emit-target-encodings", &CliOptions::emitTargetEncodings, GeneratorKind::TargetEncodings },
    { "--emit-instruction-selector", &CliOptions::emitInstructionSelector, GeneratorKind::InstructionSelector },
    { "--emit-calling-conv", &CliOptions::emitCallingConv, GeneratorKind::CallingConv },
    { "--emit-registers", &CliOptions::emitRegisterInfo, GeneratorKind::RegisterInfo },
    { "--emit-target-desc", &CliOptions::emitTargetDesc, GeneratorKind::TargetDesc },
};

// Accepted --generator spellings (case-insensitive) and the generator each selects.
struct GeneratorAlias
{
    std::string_view m_alias;  ///< Lower-case alias spelling.
    GeneratorKind m_generator; ///< Generator the alias selects.
};

constexpr GeneratorAlias kGeneratorAliases[] = {
    { "type-table", GeneratorKind::TypeTable },
    { "typetable", GeneratorKind::TypeTable },
    { "instructions", GeneratorKind::Instructions },
    { "instruction", GeneratorKind::Instructions },
    { "legalizer", GeneratorKind::Legalizer },
    { "legalize", GeneratorKind::Legalizer },
    { "rules", GeneratorKind::Rules },
    { "rule", GeneratorKind::Rules },
    { "target-instructions", GeneratorKind::TargetInstructions },
    { "target_instructions", GeneratorKind::TargetInstructions },
    { "target-inst", GeneratorKind::TargetInstructions },
    { "target-encodings", GeneratorKind::TargetEncodings },
    { "target_encodings", GeneratorKind::TargetEncodings },
    { "encodings", GeneratorKind::TargetEncodings },
    { "instruction-selector", GeneratorKind::InstructionSelector },
    { "instruction_selector", GeneratorKind::InstructionSelector },
    { "isel", GeneratorKind::InstructionSelector },
    { "calling-conv", GeneratorKind::CallingConv },
    { "calling_conv", GeneratorKind::CallingConv },
    { "callingconv", GeneratorKind::CallingConv },
    { "cc", GeneratorKind::CallingConv },
    { "registers", GeneratorKind::RegisterInfo },
    { "register", GeneratorKind::RegisterInfo },
    { "register-info", GeneratorKind::RegisterInfo },
    { "reg", GeneratorKind::RegisterInfo },
    { "target-desc", GeneratorKind::TargetDesc },
    { "target_desc", GeneratorKind::TargetDesc },
    { "targetdesc", GeneratorKind::TargetDesc },
    { "tdesc", GeneratorKind::TargetDesc },
};

// Resolves an already lowercased --generator value; Auto when unrecognized.
GeneratorKind generatorFromAlias(std::string_view alias)
{
    for (const auto &entry : kGeneratorAliases)
    {
        if (entry.m_alias == alias)
        {
            return entry.m_generator;
        }
    }
    return GeneratorKind::Auto;
}

} // namespace

// Installs all argument/option definitions used by parse().
CommandLineParser::CommandLineParser() { setupArguments(); }

// Registers every supported flag, option and positional argument with the argparse program.
void CommandLineParser::setupArguments()
{
    m_program = std::make_unique<argparse::ArgumentParser>("EzDslCli", "1.0.0", argparse::default_arguments::help);

    m_program->add_description("EzDSL Compiler Backend Driver & Code Generator Tool");

    m_program->add_argument("-i", "--input")
            .help("Path to the input EzDSL definition file (.tyf, .irdf, .lad, .lrd, .idf, .isf, .ezcc, .ccd, .reg, "
                  ".tdesc)")
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

    m_program->add_argument("--emit-target-encodings")
            .help("Synthesize Target EncodingTable (.h) from .idf ENCODING blocks")
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

    m_program->add_argument("--namespace-root")
            .help("Namespace root the generated code is emitted into (e.g. EzTargets::X86_64)")
            .metavar("<ns>")
            .default_value(std::string("EzTargets"));

    m_program->add_argument("--generator")
            .help("Explicit generator to execute: 'type-table', 'instructions', 'legalizer', 'rules', "
                  "'target-instructions', 'target-encodings', 'instruction-selector', 'calling-conv', 'registers', "
                  "'target-desc', or 'auto'")
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

// Parses argv, applies validation and resolves the requested generator; returns nullopt on help/version/error.
std::optional<CliOptions> CommandLineParser::parse(int argc, char *argv[], std::string &errorMessage)
{
    try
    {
        m_program->parse_args(argc, argv);
    }
    catch (const std::exception &err)
    {
        // argparse reports malformed usage by throwing; surface the message to the caller.
        errorMessage = err.what();
        return std::nullopt;
    }

    // --version short-circuits before any further validation.
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

    // Simple boolean and string switches map directly onto the options structure.
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

    // --format is case-insensitive; anything other than "json" falls back to text.
    std::string formatStr = StrToLower(m_program->get<std::string>("--format"));
    opts.format = formatStr == "json" ? OutputFormat::Json : OutputFormat::Text;

    opts.targetName = m_program->get<std::string>("--target");
    opts.namespaceRoot = m_program->get<std::string>("--namespace-root");
    if (opts.namespaceRoot.empty())
    {
        opts.namespaceRoot = "EzTargets";
    }

    opts.rulesFilePath = m_program->get<std::string>("--rules");
    opts.typesFilePath = m_program->get<std::string>("--types");
    opts.instructionsFilePath = m_program->get<std::string>("--instructions");

    // Emission flags: a new generator only needs an entry in kEmitOptions.
    size_t emitCount = 0;
    for (const auto &emit : kEmitOptions)
    {
        const bool present = m_program->get<bool>(emit.m_flag);
        if (emit.m_mirror)
        {
            opts.*(emit.m_mirror) = present;
        }
        if (present)
        {
            ++emitCount;
            opts.generator = emit.m_generator;
        }
    }

    // Reject ambiguous invocations that request more than one generator at once.
    if (emitCount > 1)
    {
        errorMessage = "Cannot specify multiple generator emission flags simultaneously.";
        return std::nullopt;
    }

    // Emission flags take precedence; else resolve the explicit --generator name, else Auto.
    if (emitCount == 0)
    {
        opts.generator = generatorFromAlias(StrToLower(m_program->get<std::string>("--generator")));
    }

    return opts;
}

// Returns argparse's usage/help text, or an empty string if the program was never built.
std::string CommandLineParser::getHelp() const
{
    if (!m_program)
    {
        return {};
    }
    return m_program->help().str();
}

// Returns the static tool version banner.
std::string CommandLineParser::getVersion() const { return "EzDslCli version 1.0.0 (EzPacker Compiler Suite)"; }

} // namespace Cli
