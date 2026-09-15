#include "Cli/Driver.h"
#include "Cli/InfoDumper.h"

#include "CodeGenerators/CppMirInstructionGenerator.h"
#include "CodeGenerators/CppMirTypeTableGenerator.h"

#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"

#include "Parser/IrInstructionDefLang.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "Parser/TypeDefLang.h"

#include "Sema/SymbolTable.h"
#include "SemaPasses/IrInstructionPass.h"
#include "SemaPasses/LegalizeActionPass.h"
#include "SemaPasses/LegalizeRulePass.h"
#include "SemaPasses/TypePass.h"

#include "SourceManager/SourceManager.h"

namespace Cli
{

namespace
{

class ErrorTrackingListener : public DiagnosticListener
{
  public:
    void onDiag(const DiagnosticMessage &msg) override
    {
        if (msg.getType() == Diag_Error)
        {
            ++m_errors;
        }
        else if (msg.getType() == Diag_Warning)
        {
            ++m_warnings;
        }
    }

    size_t getErrors() const noexcept { return m_errors; }
    size_t getWarnings() const noexcept { return m_warnings; }
    bool hasErrors() const noexcept { return m_errors > 0; }

  private:
    size_t m_errors{ 0 };
    size_t m_warnings{ 0 };
};

} // namespace

Driver::Driver(CliOptions options) : m_options(std::move(options))
{
}

LanguageDialect Driver::detectDialect(const std::filesystem::path &filePath) const
{
    if (m_options.dialect != LanguageDialect::Auto)
    {
        return m_options.dialect;
    }

    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

    if (ext == ".tyf")
        return LanguageDialect::TypeDef;
    if (ext == ".irdf")
        return LanguageDialect::IrInstDef;
    if (ext == ".lad")
        return LanguageDialect::LegalizeAction;
    if (ext == ".lrd")
        return LanguageDialect::LegalizeRule;

    if (m_options.generator == GeneratorKind::TypeTable)
        return LanguageDialect::TypeDef;
    if (m_options.generator == GeneratorKind::Instructions)
        return LanguageDialect::IrInstDef;

    return LanguageDialect::Auto;
}

GeneratorKind Driver::resolveGeneratorKind(LanguageDialect dialect) const
{
    if (m_options.generator != GeneratorKind::Auto)
    {
        return m_options.generator;
    }

    switch (dialect)
    {
        case LanguageDialect::TypeDef:
            return GeneratorKind::TypeTable;
        case LanguageDialect::IrInstDef:
            return GeneratorKind::Instructions;
        default:
            return GeneratorKind::Auto;
    }
}

std::vector<OutputFileInfo> Driver::computeExpectedOutputs(GeneratorKind genKind,
                                                          const std::filesystem::path &outDir) const
{
    std::vector<OutputFileInfo> outputs;

    auto resolveHeaderAndSource = [](std::filesystem::path outPath,
                                     std::string_view defaultBaseName) -> std::pair<std::filesystem::path, std::filesystem::path> {
        if (outPath.empty())
            outPath = ".";

        std::string ext = outPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

        if (ext == ".h" || ext == ".hpp")
        {
            auto sourcePath = outPath;
            sourcePath.replace_extension(".cpp");
            return { outPath, sourcePath };
        }
        if (ext == ".cpp" || ext == ".cxx" || ext == ".cc")
        {
            auto headerPath = outPath;
            headerPath.replace_extension(".h");
            return { headerPath, outPath };
        }

        return { outPath / std::format("{}.h", defaultBaseName),
                 outPath / std::format("{}.cpp", defaultBaseName) };
    };

    auto resolveSingleFile = [](std::filesystem::path outPath,
                                std::string_view defaultFileName) -> std::filesystem::path {
        if (outPath.empty())
            outPath = ".";

        std::string ext = outPath.extension().string();
        if (ext.empty() || std::filesystem::is_directory(outPath))
        {
            return outPath / defaultFileName;
        }
        return outPath;
    };

    if (genKind == GeneratorKind::TypeTable)
    {
        auto [hPath, sPath] = resolveHeaderAndSource(outDir, "MirTypeTable");

        bool emitHeader = !m_options.sourceOnly || m_options.headerOnly;
        bool emitSource = !m_options.headerOnly || m_options.sourceOnly;

        if (emitHeader)
        {
            outputs.push_back({ .role = "header", .path = hPath, .exists = std::filesystem::exists(hPath) });
        }
        if (emitSource)
        {
            outputs.push_back({ .role = "source", .path = sPath, .exists = std::filesystem::exists(sPath) });
        }
    }
    else if (genKind == GeneratorKind::Instructions)
    {
        if (!m_options.sourceOnly || m_options.headerOnly)
        {
            auto hPath = resolveSingleFile(outDir, "MirInstructionSetDefs.h");
            outputs.push_back({ .role = "header", .path = hPath, .exists = std::filesystem::exists(hPath) });
        }
    }

    return outputs;
}

DriverResult Driver::run()
{
    DriverResult result;
    std::filesystem::path inputPath(m_options.inputFilePath);

    if (!std::filesystem::exists(inputPath))
    {
        result.success = false;
        result.errorMessage = std::format("Input file '{}' does not exist.", inputPath.string());
        return result;
    }

    std::pmr::monotonic_buffer_resource arena;
    DiagnosticCollector diagCollector;

    if (m_options.verbose)
    {
        diagCollector.enableDiag(Diag_Trace);
        diagCollector.enableDiag(Diag_Debug);
    }

    SourceManager sourceManager(std::filesystem::current_path(), &arena);
    for (const auto &inc : m_options.includeDirs)
    {
        sourceManager.addIncludePath(inc);
    }

    ErrorTrackingListener errorTracker;
    diagCollector.addListener(&errorTracker);

    std::unique_ptr<DiagnosticLogger> diagLogger;
    if (!m_options.quiet)
    {
        diagLogger = std::make_unique<DiagnosticLogger>(&sourceManager);
        diagCollector.addListener(diagLogger.get());
    }

    auto sourceIdOpt = sourceManager.loadFile(inputPath);
    if (!sourceIdOpt.has_value())
    {
        result.success = false;
        result.errorMessage = std::format("Failed to load source file '{}'.", inputPath.string());
        return result;
    }
    size_t sourceId = *sourceIdOpt;

    LanguageDialect dialect = detectDialect(inputPath);
    if (dialect == LanguageDialect::Auto)
    {
        result.success = false;
        result.errorMessage = std::format("Could not determine language dialect for file '{}'. "
                                          "Please use a known extension (.tyf, .irdf, .lad, .lrd) "
                                          "or specify --generator / --emit-* option.",
                                          inputPath.string());
        return result;
    }

    GeneratorKind genKind = resolveGeneratorKind(dialect);
    std::vector<OutputFileInfo> expectedOutputs = computeExpectedOutputs(genKind, m_options.outputPath);

    // Track write timestamps of expected output files before generation
    std::unordered_map<std::string, std::filesystem::file_time_type> preModTimes;
    for (const auto &out : expectedOutputs)
    {
        std::error_code ec;
        if (std::filesystem::exists(out.path, ec))
        {
            preModTimes[out.path.string()] = std::filesystem::last_write_time(out.path, ec);
        }
    }

    ParseContext parseCtx(&diagCollector, &sourceManager, sourceId, &arena);
    SymbolTable symbolTable(&arena);
    size_t constructCount = 0;

    switch (dialect)
    {
        case LanguageDialect::TypeDef:
        {
            auto ast = parseCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
            if (!ast.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Syntax parsing failed for TypeDef file.";
                return result;
            }

            constructCount = ast->m_types.size();

            if (m_options.dumpAst)
            {
                InfoDumper::dumpTypeDefAst(*ast, m_options.format, std::cout);
            }

            TypePass pass;
            if (!pass.run(&diagCollector, &symbolTable, &ast.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Semantic analysis failed for TypeDef file.";
                return result;
            }
            break;
        }

        case LanguageDialect::IrInstDef:
        {
            auto ast = parseCtx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
            if (!ast.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Syntax parsing failed for IR Instruction Definition file.";
                return result;
            }

            constructCount = ast->m_instructions.size();

            if (m_options.dumpAst)
            {
                InfoDumper::dumpIrInstDefAst(*ast, m_options.format, std::cout);
            }

            IrInstructionPass pass;
            if (!pass.run(&diagCollector, &symbolTable, &ast.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Semantic analysis failed for IR Instruction Definition file.";
                return result;
            }
            break;
        }

        case LanguageDialect::LegalizeAction:
        {
            auto ast = parseCtx.parse<DSL::Parser::LegalizeActionDef::LegalizeActionFile,
                                      DSL::Ast::LegalizeActionDef::LegalizeActionFile>();
            if (!ast.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Syntax parsing failed for Legalization Action file.";
                return result;
            }

            constructCount = ast->m_legalizeInstrDecls.size();

            if (m_options.dumpAst)
            {
                InfoDumper::dumpLegalizeActionAst(*ast, m_options.format, std::cout);
            }

            if (!LegalizeActionPass::run(&diagCollector, &symbolTable, &ast.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Semantic analysis failed for Legalization Action file.";
                return result;
            }
            break;
        }

        case LanguageDialect::LegalizeRule:
        {
            auto ast = parseCtx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRuleFile,
                                      DSL::Ast::LegalizeRuleDef::LegalizeRuleFile>();
            if (!ast.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Syntax parsing failed for Legalization Rule file.";
                return result;
            }

            constructCount = ast->m_rules.size();

            if (m_options.dumpAst)
            {
                InfoDumper::dumpLegalizeRuleAst(*ast, m_options.format, std::cout);
            }

            if (!LegalizeRulePass::run(&diagCollector, &symbolTable, &ast.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Semantic analysis failed for Legalization Rule file.";
                return result;
            }
            break;
        }

        default:
            break;
    }

    if (m_options.dumpSymbols)
    {
        InfoDumper::dumpSymbols(symbolTable, m_options.format, std::cout);
    }

    if (m_options.dumpFiles)
    {
        InfoDumper::dumpOutputFiles(expectedOutputs, m_options.format, std::cout);
    }

    if (m_options.dumpInfo)
    {
        GeneralFileInfo gInfo;
        gInfo.inputPath = inputPath;
        std::error_code ec;
        gInfo.fileSizeBytes = std::filesystem::file_size(inputPath, ec);
        gInfo.dialect = dialect;
        switch (dialect)
        {
            case LanguageDialect::TypeDef: gInfo.dialectName = "TypeDef (.tyf)"; break;
            case LanguageDialect::IrInstDef: gInfo.dialectName = "IrInstDef (.irdf)"; break;
            case LanguageDialect::LegalizeAction: gInfo.dialectName = "LegalizeAction (.lad)"; break;
            case LanguageDialect::LegalizeRule: gInfo.dialectName = "LegalizeRule (.lrd)"; break;
            default: gInfo.dialectName = "Unknown"; break;
        }
        gInfo.constructCount = constructCount;

        switch (genKind)
        {
            case GeneratorKind::TypeTable: gInfo.generatorName = "CppMirTypeTableGenerator"; break;
            case GeneratorKind::Instructions: gInfo.generatorName = "CppMirInstructionGenerator"; break;
            default: gInfo.generatorName = "None"; break;
        }

        if (m_options.headerOnly && !m_options.sourceOnly)
            gInfo.workingMode = "Header-only (.h)";
        else if (m_options.sourceOnly && !m_options.headerOnly)
            gInfo.workingMode = "Source-only (.cpp)";
        else
            gInfo.workingMode = "Full (.h and .cpp)";

        gInfo.outputDirectory = m_options.outputPath;
        gInfo.outputs = expectedOutputs;

        InfoDumper::dumpGeneralInfo(gInfo, m_options.format, std::cout);
    }

    if (m_options.checkOnly || m_options.dryRun)
    {
        result.success = true;
        return result;
    }

    // Only run code generation if a recognized code generator is requested or auto-selected
    if (genKind == GeneratorKind::TypeTable)
    {
        using namespace CodeGenerators;
        MirTypeTableGenWorkingMode mode = MirTypeTableGenWorkingMode::Full;
        if (m_options.headerOnly && !m_options.sourceOnly)
        {
            mode = MirTypeTableGenWorkingMode::Header;
        }
        else if (m_options.sourceOnly && !m_options.headerOnly)
        {
            mode = MirTypeTableGenWorkingMode::Source;
        }

        CppMirTypeTableGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, mode);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during MirTypeTable synthesis.";
            return result;
        }
    }
    else if (genKind == GeneratorKind::Instructions)
    {
        using namespace CodeGenerators;
        if (m_options.sourceOnly && !m_options.headerOnly)
        {
            if (!m_options.quiet)
            {
                std::cerr << "Warning: --source-only requested for instruction definitions, "
                             "but instructions only synthesize a C++ header.\n";
            }
        }
        else
        {
            CppMirInstructionGenerator generator(&diagCollector, &symbolTable, m_options.outputPath);
            if (!generator.run() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Code generation failed during MirInstructionSetDefs synthesis.";
                return result;
            }
        }
    }
    else
    {
        // No generator available (e.g. .lad or .lrd without code generators yet)
        if (!m_options.dumpFiles && !m_options.dumpAst && !m_options.dumpSymbols && !m_options.dumpInfo)
        {
            result.success = false;
            result.errorMessage = "No code generator is available for the specified dialect. "
                                  "Use --check-only or dump options to inspect semantics.";
            return result;
        }
    }

    // Determine generated vs unchanged files
    for (const auto &out : expectedOutputs)
    {
        std::error_code ec;
        if (std::filesystem::exists(out.path, ec))
        {
            auto currentMod = std::filesystem::last_write_time(out.path, ec);
            auto it = preModTimes.find(out.path.string());
            if (it == preModTimes.end() || it->second != currentMod)
            {
                result.generatedFiles.push_back(out);
            }
            else
            {
                result.unchangedFiles.push_back(out);
            }
        }
    }

    if (!m_options.quiet && (!result.generatedFiles.empty() || !result.unchangedFiles.empty()))
    {
        std::cout << "[EzDSL] Code generation finished successfully.\n";
        for (const auto &f : result.generatedFiles)
        {
            std::cout << std::format("  Synthesized: [{}] {}\n", f.role, f.path.string());
        }
        for (const auto &f : result.unchangedFiles)
        {
            std::cout << std::format("  Up-to-date:  [{}] {}\n", f.role, f.path.string());
        }
    }

    result.success = true;
    return result;
}

} // namespace Cli
