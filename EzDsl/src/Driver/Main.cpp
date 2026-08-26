#include "EzDslCommon.h"

#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/TargetDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "CodeGenerators/CppMirInstructionGenerator.h"
#include "CodeGenerators/CppMirTypeTableGenerator.h"
#include "CodeGenerators/CppTargetBankGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Parser/TypeDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/IrInstructionPass.h"
#include "SemaPasses/RegisterBankPass.h"
#include "SemaPasses/TypePass.h"
#include "SourceManager/SourceManager.h"

namespace
{

struct CliOptions
{
    std::filesystem::path m_inputFile;
    std::filesystem::path m_outputPath{ "." };
    std::vector<std::filesystem::path> m_includePaths;
    std::string m_targetName;

    bool m_emitTypeTable{ false };
    bool m_emitInstructions{ false };
    bool m_emitRegisterBanks{ false };
    CodeGenerators::MirTypeTableGenWorkingMode m_genMode{ CodeGenerators::MirTypeTableGenWorkingMode::Full };
    CodeGenerators::TargetBankGenWorkingMode m_bankGenMode{ CodeGenerators::TargetBankGenWorkingMode::Full };

    bool m_verbose{ false };
    bool m_showHelp{ false };
    bool m_showVersion{ false };
};

void PrintVersion() { std::cout << "ezdsl-gen - EzDSL Compiler Backend Generator (v0.1.0)\n"; }

void PrintHelp(std::string_view programName)
{
    std::cout << std::format("Usage: {} [options] -i <input_file>\n\n"
                             "Options:\n"
                             "  -i, --input <file>        Input EzDSL definition file (.tyf, .irdf, .tdf, .idf)\n"
                             "  -o, --output <path>       Output path (directory or root file name, default: .)\n"
                             "  -I, --include <dir>       Add directory to search paths for file inclusions\n"
                             "  --target <name>           Specify target architecture name\n"
                             "  --emit-type-table         Synthesize EzMir TypeTable source and header files\n"
                             "  --emit-instructions       Synthesize EzMir IR instruction definition file\n"
                             "  --emit-register-banks     Synthesize target register banks and classes files\n"
                             "  --header-only             Emit only the header file (.h) during generation\n"
                             "  --source-only             Emit only the translation unit (.cpp) during generation\n"
                             "  -v, --verbose             Enable verbose tracing output\n"
                             "  -h, --help                Display this help message\n"
                             "  --version                 Display version information\n",
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

        // Positional argument fallback
        if (opts.m_inputFile.empty() && !arg.empty() && arg[0] != '-')
        {
            opts.m_inputFile = arg;
            continue;
        }

        std::cerr << "Error: Unrecognized option: " << arg << "\n";
        return std::nullopt;
    }

    if (opts.m_inputFile.empty() && !opts.m_showHelp && !opts.m_showVersion)
    {
        std::cerr << "Error: No input file specified. Use -i <file> or --help.\n";
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

    // Initialize PMR Monotonic Buffer Arena (16MB scratchpad)
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

    // Load Source File Buffer
    auto fileId = sourceManager.loadFile(options->m_inputFile);
    if (!fileId)
    {
        collector.error("Driver", "Failed to open or load source file: {}", options->m_inputFile.string());
        return EXIT_FAILURE;
    }

    // Initialize Parser Context
    ParseContext parseCtx(&collector, &sourceManager, fileId.value(), &arena);

    std::string_view extension = options->m_inputFile.extension().string();
    SymbolTable symbolTable(&arena);

    if (extension == ".tyf")
    {
        auto ast = parseCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
        if (!ast)
        {
            collector.error("Driver", "Failed to parse Type Definition file: {}", options->m_inputFile.string());
            return EXIT_FAILURE;
        }

        TypePass typePass;
        if (!typePass.run(&collector, &symbolTable, &*ast))
        {
            collector.error("Driver", "Semantic analysis failed for: {}", options->m_inputFile.string());
            return EXIT_FAILURE;
        }

        if (options->m_emitTypeTable)
        {
            if (!CodeGenerators::GenerateMirTypeTable(&collector,
                                                      &symbolTable,
                                                      options->m_outputPath,
                                                      options->m_genMode))
            {
                collector.error("Driver", "Code generation failed for MirTypeTable.");
                return EXIT_FAILURE;
            }
        }
    }
    else if (extension == ".irdf" || extension == ".iid")
    {
        auto ast = parseCtx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
        if (!ast)
        {
            collector.error("Driver",
                            "Failed to parse IR Instruction Definition file: {}",
                            options->m_inputFile.string());
            return EXIT_FAILURE;
        }

        IrInstructionPass instPass;
        if (!instPass.run(&collector, &symbolTable, &*ast))
        {
            collector.error("Driver", "Semantic analysis failed for: {}", options->m_inputFile.string());
            return EXIT_FAILURE;
        }

        if (options->m_emitInstructions)
        {
            if (!CodeGenerators::GenerateMirIrInstructionDefs(&collector, &symbolTable, options->m_outputPath))
            {
                collector.error("Driver", "Code generation failed for MirInstructionSetDefs.");
                return EXIT_FAILURE;
            }
        }
    }
    else if (extension == ".tdf")
    {
        auto ast = parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
        if (!ast)
        {
            collector.error("Driver", "Failed to parse Target Definition file: {}", options->m_inputFile.string());
            return EXIT_FAILURE;
        }

        RegisterBankPass bankPass;
        if (!bankPass.run(&collector, &symbolTable, &*ast))
        {
            collector.error("Driver", "Semantic analysis failed for: {}", options->m_inputFile.string());
            return EXIT_FAILURE;
        }

        if (options->m_emitRegisterBanks || (!options->m_emitTypeTable && !options->m_emitInstructions))
        {
            std::string targetName =
                    options->m_targetName.empty() ? std::string(ast->m_name.m_node) : options->m_targetName;
            if (!CodeGenerators::GenerateTargetRegisterBanks(&collector,
                                                             &symbolTable,
                                                             options->m_outputPath,
                                                             targetName,
                                                             options->m_bankGenMode))
            {
                collector.error("Driver", "Code generation failed for TargetRegisterBanks.");
                return EXIT_FAILURE;
            }
        }
    }
    else
    {
        collector.error("Driver", "Unsupported file extension '{}'. Supported: .tyf, .irdf, .tdf", extension);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}