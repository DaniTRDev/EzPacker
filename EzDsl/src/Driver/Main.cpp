#include "EzDslCommon.h"

#include "Ast/CallingConvDefLangAst.h"
#include "Ast/InstructionDefLangAst.h"
#include "Ast/InstructionSelDefLangAst.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/TargetDefLangAst.h"
#include "Ast/TypeDefLangAst.h"

#include "CodeGenerators/CppCallingConvGenerator.h"
#include "CodeGenerators/CppISelTableGenerator.h"
#include "CodeGenerators/CppLegalizerGenerator.h"
#include "CodeGenerators/CppLegalizerRuleGenerator.h"
#include "CodeGenerators/CppMirInstructionGenerator.h"
#include "CodeGenerators/CppMirTypeTableGenerator.h"
#include "CodeGenerators/CppTargetBankGenerator.h"
#include "CodeGenerators/CppTargetDescGenerator.h"
#include "CodeGenerators/CppTargetInstGenerator.h"
#include "CodeGenerators/CppTargetTypeLayoutGenerator.h"

#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"

#include "Parser/CallingConvDefLang.h"
#include "Parser/InstructionDefLang.h"
#include "Parser/InstructionSelDefLang.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Parser/TypeDefLang.h"

#include "Sema/SymbolTable.h"
#include "SemaPasses/CallingConvPass.h"
#include "SemaPasses/InstSelPass.h"
#include "SemaPasses/IrInstructionPass.h"
#include "SemaPasses/LegalizeActionPass.h"
#include "SemaPasses/LegalizeRulePass.h"
#include "SemaPasses/RegisterBankPass.h"
#include "SemaPasses/TargetDefPass.h"
#include "SemaPasses/TargetInstPass.h"
#include "SemaPasses/TypePass.h"
#include "SourceManager/SourceManager.h"

namespace
{

struct CliOptions
{
    std::filesystem::path m_inputFile;
    std::filesystem::path m_outputPath{ "." };
    std::vector<std::filesystem::path> m_includePaths;

    // Multi-file target pipeline inputs
    std::filesystem::path m_tdfFile;
    std::filesystem::path m_idfFile;
    std::filesystem::path m_ladFile;
    std::filesystem::path m_lrdFile;
    std::filesystem::path m_isfFile;
    std::filesystem::path m_ccdfFile;
    std::filesystem::path m_tyfFile;
    std::filesystem::path m_irdfFile;

    std::string m_targetName;

    bool m_emitAll{ false };
    bool m_emitTypeTable{ false };
    bool m_emitInstructions{ false };
    bool m_emitRegisterBanks{ false };
    bool m_emitTargetInsts{ false };
    bool m_emitTypeLayout{ false };
    bool m_emitLegalizer{ false };
    bool m_emitLegalizeRules{ false };
    bool m_emitISel{ false };
    bool m_emitCallingConvs{ false };
    bool m_emitTargetDesc{ false };

    CodeGenerators::MirTypeTableGenWorkingMode m_genMode{ CodeGenerators::MirTypeTableGenWorkingMode::Full };
    CodeGenerators::TargetBankGenWorkingMode m_bankGenMode{ CodeGenerators::TargetBankGenWorkingMode::Full };

    bool m_verbose{ false };
    bool m_showHelp{ false };
    bool m_showVersion{ false };
};

void PrintVersion() { std::cout << "ezdsl-gen - EzDSL Compiler Backend Generator (v0.1.0)\n"; }

void PrintHelp(std::string_view programName)
{
    std::cout << std::format("Usage: {} [options] -i <input_file>\n"
                             "   or: {} --target <Name> --tdf <file> --idf <file> ... -o <dir>\n\n"
                             "Pipeline Target Options:\n"
                             "  --target <name>           Specify target architecture name\n"
                             "  --tdf <file>              Target register bank definition file\n"
                             "  --idf <file>              Target instruction definition file\n"
                             "  --lad <file>              Legalize action definition file\n"
                             "  --lrd <file>              Legalize rule definition file\n"
                             "  --isf <file>              Instruction selection definition file\n"
                             "  --ccdf <file>             Calling convention definition file\n"
                             "  --tyf <file>              Type table definition file\n"
                             "  --irdf <file>             IR instruction definition file\n"
                             "  --emit-all                Emit all target backend components\n\n"
                             "Individual Emitters & Generic Options:\n"
                             "  -i, --input <file>        Input single EzDSL definition file\n"
                             "  -o, --output <path>       Output path (directory or root file name, default: .)\n"
                             "  -I, --include <dir>       Add directory to search paths for file inclusions\n"
                             "  --emit-type-table         Synthesize EzMir TypeTable files\n"
                             "  --emit-instructions       Synthesize EzMir IR instruction definitions\n"
                             "  --emit-register-banks     Synthesize target register banks\n"
                             "  --header-only             Emit only header files (.h)\n"
                             "  --source-only             Emit only translation units (.cpp)\n"
                             "  -v, --verbose             Enable verbose tracing output\n"
                             "  -h, --help                Display this help message\n"
                             "  --version                 Display version information\n",
                             programName,
                             programName);
}

std::optional<CliOptions> ParseCommandLine(int argc, char **argv)
{
    CliOptions opts;
    std::span<char *> args(argv + 1, argc - 1);

    for (size_t i = 0; i < args.size(); ++i)
    {
        std::string_view arg = args[i];

        if (arg == "-h" || arg == "--help")
        {
            opts.m_showHelp = true;
            return opts;
        }
        if (arg == "--version")
        {
            opts.m_showVersion = true;
            return opts;
        }
        if (arg == "-v" || arg == "--verbose")
        {
            opts.m_verbose = true;
            continue;
        }
        if (arg == "--emit-all")
        {
            opts.m_emitAll = true;
            continue;
        }
        if (arg == "--emit-type-table")
        {
            opts.m_emitTypeTable = true;
            continue;
        }
        if (arg == "--emit-instructions" || arg == "--emit-ir-instructions")
        {
            opts.m_emitInstructions = true;
            continue;
        }
        if (arg == "--emit-register-banks" || arg == "--emit-registers" || arg == "--emit-banks")
        {
            opts.m_emitRegisterBanks = true;
            continue;
        }
        if (arg == "--target")
        {
            if (++i >= args.size())
            {
                std::cerr << "Error: Missing argument for option: " << arg << "\n";
                return std::nullopt;
            }
            opts.m_targetName = args[i];
            continue;
        }
        if (arg == "--tdf")
        {
            if (++i >= args.size()) { return std::nullopt; }
            opts.m_tdfFile = args[i];
            continue;
        }
        if (arg == "--idf")
        {
            if (++i >= args.size()) { return std::nullopt; }
            opts.m_idfFile = args[i];
            continue;
        }
        if (arg == "--lad")
        {
            if (++i >= args.size()) { return std::nullopt; }
            opts.m_ladFile = args[i];
            continue;
        }
        if (arg == "--lrd")
        {
            if (++i >= args.size()) { return std::nullopt; }
            opts.m_lrdFile = args[i];
            continue;
        }
        if (arg == "--isf")
        {
            if (++i >= args.size()) { return std::nullopt; }
            opts.m_isfFile = args[i];
            continue;
        }
        if (arg == "--ccdf")
        {
            if (++i >= args.size()) { return std::nullopt; }
            opts.m_ccdfFile = args[i];
            continue;
        }
        if (arg == "--tyf")
        {
            if (++i >= args.size()) { return std::nullopt; }
            opts.m_tyfFile = args[i];
            continue;
        }
        if (arg == "--irdf")
        {
            if (++i >= args.size()) { return std::nullopt; }
            opts.m_irdfFile = args[i];
            continue;
        }
        if (arg == "--header-only")
        {
            opts.m_genMode = CodeGenerators::MirTypeTableGenWorkingMode::Header;
            opts.m_bankGenMode = CodeGenerators::TargetBankGenWorkingMode::Header;
            continue;
        }
        if (arg == "--source-only")
        {
            opts.m_genMode = CodeGenerators::MirTypeTableGenWorkingMode::Source;
            opts.m_bankGenMode = CodeGenerators::TargetBankGenWorkingMode::Source;
            continue;
        }
        if (arg == "-i" || arg == "--input")
        {
            if (++i >= args.size())
            {
                std::cerr << "Error: Missing argument for option: " << arg << "\n";
                return std::nullopt;
            }
            opts.m_inputFile = args[i];
            continue;
        }
        if (arg == "-o" || arg == "--output")
        {
            if (++i >= args.size())
            {
                std::cerr << "Error: Missing argument for option: " << arg << "\n";
                return std::nullopt;
            }
            opts.m_outputPath = args[i];
            continue;
        }
        if (arg == "-I" || arg == "--include")
        {
            if (++i >= args.size())
            {
                std::cerr << "Error: Missing argument for option: " << arg << "\n";
                return std::nullopt;
            }
            opts.m_includePaths.emplace_back(args[i]);
            continue;
        }

        // Positional fallback
        if (opts.m_inputFile.empty() && !arg.empty() && arg[0] != '-')
        {
            opts.m_inputFile = arg;
            continue;
        }

        std::cerr << "Error: Unrecognized option: " << arg << "\n";
        return std::nullopt;
    }

    if (opts.m_inputFile.empty() && opts.m_tdfFile.empty() && !opts.m_showHelp && !opts.m_showVersion)
    {
        std::cerr << "Error: No input files specified. Use -i <file> or target pipeline options (--help).\n";
        return std::nullopt;
    }

    return opts;
}

} // namespace

int main(int argc, char **argv)
{
    auto options = ParseCommandLine(argc, argv);
    if (!options)
    {
        return EXIT_FAILURE;
    }

    if (options->m_showHelp)
    {
        PrintHelp(argv[0]);
        return EXIT_SUCCESS;
    }

    if (options->m_showVersion)
    {
        PrintVersion();
        return EXIT_SUCCESS;
    }

    // Initialize PMR Arena (16MB scratchpad)
    constexpr size_t ArenaSize = 16 * 1024 * 1024;
    auto arenaBuffer = std::make_unique<std::byte[]>(ArenaSize);
    std::pmr::monotonic_buffer_resource arena(arenaBuffer.get(), ArenaSize);

    DiagnosticCollector collector;
    SourceManager sourceManager(std::filesystem::current_path(), &arena);
    DiagnosticLogger logger(&sourceManager);

    collector.addListener(&logger);

    if (options->m_verbose)
    {
        collector.enableDiag(DiagnosticMessageType::Diag_Trace);
    }

    for (const auto &includeDir : options->m_includePaths)
    {
        sourceManager.addIncludePath(includeDir);
    }

    SymbolTable symbolTable(&arena);

    // ========================================================================
    // Multi-File Target Pipeline Mode
    // ========================================================================
    if (!options->m_tdfFile.empty() || options->m_emitAll)
    {
        // 1. Types (.tyf)
        if (!options->m_tyfFile.empty())
        {
            auto fId = sourceManager.loadFile(options->m_tyfFile);
            if (fId)
            {
                ParseContext pCtx(&collector, &sourceManager, *fId, &arena);
                auto ast = pCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
                if (ast)
                {
                    TypePass typePass;
                    typePass.run(&collector, &symbolTable, &*ast);
                }
            }
        }

        // 2. IR Instructions (.irdf)
        if (!options->m_irdfFile.empty())
        {
            auto fId = sourceManager.loadFile(options->m_irdfFile);
            if (fId)
            {
                ParseContext pCtx(&collector, &sourceManager, *fId, &arena);
                auto ast = pCtx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
                if (ast)
                {
                    IrInstructionPass instPass;
                    instPass.run(&collector, &symbolTable, &*ast);
                }
            }
        }

        // 3. Target Definition (.tdf)
        if (!options->m_tdfFile.empty())
        {
            auto fId = sourceManager.loadFile(options->m_tdfFile);
            if (fId)
            {
                ParseContext pCtx(&collector, &sourceManager, *fId, &arena);
                auto ast = pCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
                if (ast)
                {
                    TargetDefPass::run(&collector, &symbolTable, &*ast);
                    RegisterBankPass::run(&collector, &symbolTable, &*ast);
                    if (options->m_targetName.empty())
                    {
                        options->m_targetName = std::string(ast->m_name.m_node);
                    }
                }
            }
        }

        // 4. Target Instructions (.idf)
        if (!options->m_idfFile.empty())
        {
            auto fId = sourceManager.loadFile(options->m_idfFile);
            if (fId)
            {
                ParseContext pCtx(&collector, &sourceManager, *fId, &arena);
                auto ast = pCtx.parse<DSL::Parser::InstDef::InstDefFile, DSL::Ast::InstDef::InstDefFile>();
                if (ast)
                {
                    InstructionDefPass::run(&collector, &symbolTable, &*ast);
                }
            }
        }

        // 5. Legalize Actions (.lad)
        if (!options->m_ladFile.empty())
        {
            auto fId = sourceManager.loadFile(options->m_ladFile);
            if (fId)
            {
                ParseContext pCtx(&collector, &sourceManager, *fId, &arena);
                auto ast = pCtx.parse<DSL::Parser::LegalizeActionDef::TargetLegalizeDef, DSL::Ast::LegalizeActionDef::TargetLegalizeDef>();
                if (ast)
                {
                    LegalizeActionPass::run(&collector, &symbolTable, &*ast);
                }
            }
        }

        // 6. Legalize Rules (.lrd)
        if (!options->m_lrdFile.empty())
        {
            auto fId = sourceManager.loadFile(options->m_lrdFile);
            if (fId)
            {
                ParseContext pCtx(&collector, &sourceManager, *fId, &arena);
                auto ast = pCtx.parse<DSL::Parser::LegalizeRuleDef::TargetLegalizeRuleDef, DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef>();
                if (ast)
                {
                    LegalizeRulePass::run(&collector, &symbolTable, &*ast);
                }
            }
        }

        // 7. Instruction Selection (.isf)
        if (!options->m_isfFile.empty())
        {
            auto fId = sourceManager.loadFile(options->m_isfFile);
            if (fId)
            {
                ParseContext pCtx(&collector, &sourceManager, *fId, &arena);
                auto ast = pCtx.parse<DSL::Parser::InstSelDef::ISelDefFileParser, DSL::Ast::InstSelDef::ISelDefFile>();
                if (ast)
                {
                    InstSelPass::run(&collector, &symbolTable, &*ast);
                }
            }
        }

        // 8. Calling Conventions (.ccdf)
        if (!options->m_ccdfFile.empty())
        {
            auto fId = sourceManager.loadFile(options->m_ccdfFile);
            if (fId)
            {
                ParseContext pCtx(&collector, &sourceManager, *fId, &arena);
                auto ast = pCtx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile, DSL::Ast::CallingConvDef::CallingConvDefFile>();
                if (ast)
                {
                    CallingConvPass::run(&collector, &symbolTable, &*ast);
                }
            }
        }

        // Emit all target backend code generators
        std::string targetName = options->m_targetName.empty() ? "Target" : options->m_targetName;

        CodeGenerators::GenerateTargetRegisterBanks(&collector, &symbolTable, options->m_outputPath, targetName);
        CodeGenerators::GenerateTargetInstructionDefs(&collector, &symbolTable, options->m_outputPath, targetName);
        CodeGenerators::GenerateTargetTypeLayout(&collector, &symbolTable, options->m_outputPath, targetName);
        CodeGenerators::GenerateTargetLegalizerTable(&collector, &symbolTable, options->m_outputPath, targetName);
        CodeGenerators::GenerateTargetLegalizerRules(&collector, &symbolTable, options->m_outputPath, targetName);
        CodeGenerators::GenerateTargetISelTable(&collector, &symbolTable, options->m_outputPath, targetName);
        CodeGenerators::GenerateTargetCallingConventions(&collector, &symbolTable, options->m_outputPath, targetName);
        CodeGenerators::GenerateTargetDescriptor(&collector, &symbolTable, options->m_outputPath, targetName);

        return EXIT_SUCCESS;
    }

    // ========================================================================
    // Single-File Mode Fallback
    // ========================================================================
    auto fileId = sourceManager.loadFile(options->m_inputFile);
    if (!fileId)
    {
        collector.error("Driver", "Failed to open or load source file: {}", options->m_inputFile.string());
        return EXIT_FAILURE;
    }

    ParseContext parseCtx(&collector, &sourceManager, fileId.value(), &arena);
    std::string_view extension = options->m_inputFile.extension().string();

    if (extension == ".tyf")
    {
        auto ast = parseCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
        if (!ast) { return EXIT_FAILURE; }
        TypePass typePass;
        if (!typePass.run(&collector, &symbolTable, &*ast)) { return EXIT_FAILURE; }
        if (options->m_emitTypeTable)
        {
            CodeGenerators::GenerateMirTypeTable(&collector, &symbolTable, options->m_outputPath, options->m_genMode);
        }
    }
    else if (extension == ".irdf" || extension == ".iid")
    {
        auto ast = parseCtx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
        if (!ast) { return EXIT_FAILURE; }
        IrInstructionPass instPass;
        if (!instPass.run(&collector, &symbolTable, &*ast)) { return EXIT_FAILURE; }
        if (options->m_emitInstructions)
        {
            CodeGenerators::GenerateMirIrInstructionDefs(&collector, &symbolTable, options->m_outputPath);
        }
    }
    else if (extension == ".tdf")
    {
        auto ast = parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
        if (!ast) { return EXIT_FAILURE; }
        if (!TargetDefPass::run(&collector, &symbolTable, &*ast)) { return EXIT_FAILURE; }
        if (!RegisterBankPass::run(&collector, &symbolTable, &*ast)) { return EXIT_FAILURE; }

        std::string targetName = options->m_targetName.empty() ? std::string(ast->m_name.m_node) : options->m_targetName;
        CodeGenerators::GenerateTargetRegisterBanks(&collector, &symbolTable, options->m_outputPath, targetName, options->m_bankGenMode);
    }
    else
    {
        collector.error("Driver", "Unsupported file extension '{}'", extension);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}