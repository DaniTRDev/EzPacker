#include "Cli/Driver.h"
#include "Cli/InfoDumper.h"

#include <cctype>

#include "CodeGenerators/CppLegalizeRuleGenerator.h"
#include "CodeGenerators/CppLegalizerGenerator.h"
#include "CodeGenerators/CppMirInstructionGenerator.h"
#include "CodeGenerators/CppMirTypeTableGenerator.h"
#include "CodeGenerators/CppTargetInstructionGenerator.h"
#include "CodeGenerators/CppEncodingTableGenerator.h"
#include "CodeGenerators/CppInstructionSelectorGenerator.h"
#include "CodeGenerators/CppCallingConvGenerator.h"
#include "CodeGenerators/CppRegisterInfoGenerator.h"
#include "CodeGenerators/CppTargetDescGenerator.h"
#include "CodeGenerators/CodeGenerator.h"

#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"

#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Ast/TargetInstDefLangAst.h"
#include "Ast/InstructionSelectDefLangAst.h"
#include "Ast/CallingConvDefLangAst.h"
#include "Ast/TargetDescDefLangAst.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "Parser/TypeDefLang.h"
#include "Parser/TargetInstDefLang.h"
#include "Parser/InstructionSelectDefLang.h"
#include "Parser/CallingConvDefLang.h"
#include "Parser/TargetDescDefLang.h"

#include "Sema/SymbolTable.h"
#include "Sema/Symbols/IrSymbols.h"
#include "Sema/Symbols/TypeSymbols.h"
#include "Sema/Symbols/TargetSymbols.h"
#include "Sema/Symbols/InstructionSelectSymbols.h"
#include "Sema/Symbols/CallingConvSymbols.h"
#include "Sema/Symbols/RegisterSymbols.h"
#include "Sema/Symbols/TargetDescSymbols.h"
#include "SemaPasses/IrInstructionPass.h"
#include "SemaPasses/LegalizeActionPass.h"
#include "SemaPasses/LegalizeRulePass.h"
#include "SemaPasses/TypePass.h"
#include "SemaPasses/TargetInstPass.h"
#include "SemaPasses/InstructionSelectPass.h"
#include "SemaPasses/CallingConvPass.h"
#include "SemaPasses/TargetDescPass.h"

#include "SourceManager/SourceManager.h"

namespace Cli
{

namespace
{

// Diagnostic listener that counts errors and warnings emitted during a run.
class ErrorTrackingListener : public DiagnosticListener
{
  public:
    // Tallies each diagnostic by severity as it is published.
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

    /** Returns the number of error diagnostics observed. */
    size_t getErrors() const noexcept { return m_errors; }

    /** Returns the number of warning diagnostics observed. */
    size_t getWarnings() const noexcept { return m_warnings; }

    /** Returns true if at least one error diagnostic was observed. */
    bool hasErrors() const noexcept { return m_errors > 0; }

  private:
    size_t m_errors{ 0 };   ///< Count of Diag_Error messages.
    size_t m_warnings{ 0 }; ///< Count of Diag_Warning messages.
};

// Declares the built-in primitive types (integers, floats, pointer, void, ...) in the symbol table.
void initializeStandardTypes(SymbolTable &table)
{
    // Compile-time description of one built-in type.
    struct BuiltinType
    {
        std::string_view name;            ///< DSL type name.
        DSL::Ast::TypeDef::TypeKind kind; ///< Classification of the type.
        uint32_t bitWidth;                ///< Width in bits.
        uint32_t alignment;               ///< ABI alignment in bits.
    };

    static constexpr BuiltinType builtinTypes[] = { { "_void", DSL::Ast::TypeDef::TypeKind::Void, 0, 0 },
                                                    { "__bindToken", DSL::Ast::TypeDef::TypeKind::BindingToken, 0, 0 },
                                                    { "ptr", DSL::Ast::TypeDef::TypeKind::Pointer, 0, 0 },
                                                    { "i1", DSL::Ast::TypeDef::TypeKind::Integer, 1, 1 },
                                                    { "i8", DSL::Ast::TypeDef::TypeKind::Integer, 8, 8 },
                                                    { "i16", DSL::Ast::TypeDef::TypeKind::Integer, 16, 16 },
                                                    { "i32", DSL::Ast::TypeDef::TypeKind::Integer, 32, 32 },
                                                    { "i64", DSL::Ast::TypeDef::TypeKind::Integer, 64, 64 },
                                                    { "i128", DSL::Ast::TypeDef::TypeKind::Integer, 128, 128 },
                                                    { "i256", DSL::Ast::TypeDef::TypeKind::Integer, 256, 256 },
                                                    { "f32", DSL::Ast::TypeDef::TypeKind::FloatingPoint, 32, 32 },
                                                    { "f64", DSL::Ast::TypeDef::TypeKind::FloatingPoint, 64, 64 },
                                                    { "f128", DSL::Ast::TypeDef::TypeKind::FloatingPoint, 128, 128 } };

    // Assign compact ids in declaration order, skipping names already provided by the input.
    uint8_t compactId = 1;
    for (const auto &t : builtinTypes)
    {
        if (table.getSymByName(t.name) != nullptr)
        {
            compactId++;
            continue;
        }

        Symbols::TypeSymbol symData{ .m_name = t.name,
                                     .m_kind = t.kind,
                                     .m_bitWidth = t.bitWidth,
                                     .m_alignment = t.alignment,
                                     .m_compactId = compactId++ };
        table.declareSym(nullptr, SymbolType::Type, std::move(symData), t.name);
    }
}

// Declares a fallback IR instruction unless a symbol of the same name already exists.
void registerFallbackIrInstruction(SymbolTable &table,
                                   std::string_view name,
                                   DSL::Ast::IrInstDef::IrInstCategory category,
                                   DSL::Ast::IrInstDef::IrInstTier tier,
                                   DSL::Ast::IrInstDef::IrInstFlag flags,
                                   std::initializer_list<Symbols::IrOperandSymbol> operands)
{
    if (table.getSymByName(name) != nullptr)
        return;

    std::pmr::vector<Symbols::IrOperandSymbol> semaOperands(table.getAllocator());
    for (const auto &op : operands)
    {
        semaOperands.push_back(op);
    }

    Symbols::IrInstructionSymbol data{ .m_name = name,
                                       .m_category = category,
                                       .m_tier = tier,
                                       .m_flags = flags,
                                       .m_operands = std::move(semaOperands) };

    table.declareSym(nullptr, SymbolType::IrInstruction, std::move(data), name);
}

// Populates the symbol table with the baseline IR instruction set used when no .irdf is supplied.
void initializeStandardIrInstructions(SymbolTable &table)
{
    using namespace DSL::Ast::IrInstDef;

    // Declares a three-operand binary arithmetic op with the SizeMatch flag preset.
    auto binaryAlu = [&](std::string_view name, IrInstFlag extraFlags = IrInstFlag::None)
    {
        registerFallbackIrInstruction(table,
                                      name,
                                      IrInstCategory::Arithmetic,
                                      IrInstTier::HighLevel,
                                      static_cast<IrInstFlag>(static_cast<uint32_t>(IrInstFlag::SizeMatch) |
                                                              static_cast<uint32_t>(extraFlags)),
                                      { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                                        { IrOperandType::Register, "lhs", IrOperandDir::ArgIn },
                                        { IrOperandType::RegImm, "rhs", IrOperandDir::ArgIn } });
    };

    // Declares a three-operand binary bitwise op taking a register-or-integer-immediate rhs.
    auto binaryBitwise = [&](std::string_view name, IrInstFlag extraFlags = IrInstFlag::None)
    {
        registerFallbackIrInstruction(table,
                                      name,
                                      IrInstCategory::Bitwise,
                                      IrInstTier::HighLevel,
                                      static_cast<IrInstFlag>(static_cast<uint32_t>(IrInstFlag::SizeMatch) |
                                                              static_cast<uint32_t>(extraFlags)),
                                      { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                                        { IrOperandType::Register, "lhs", IrOperandDir::ArgIn },
                                        { IrOperandType::RegIntImm, "rhs", IrOperandDir::ArgIn } });
    };

    // Declares a shift op taking a value and a register-or-immediate amount.
    auto shiftOp = [&](std::string_view name, IrInstFlag extraFlags = IrInstFlag::None)
    {
        registerFallbackIrInstruction(table,
                                      name,
                                      IrInstCategory::Bitwise,
                                      IrInstTier::HighLevel,
                                      extraFlags,
                                      { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                                        { IrOperandType::Register, "val", IrOperandDir::ArgIn },
                                        { IrOperandType::RegIntImm, "amt", IrOperandDir::ArgIn } });
    };

    // Declares a comparison op producing a destination plus lhs/rhs inputs.
    auto compareOp = [&](std::string_view name, IrInstFlag extraFlags = IrInstFlag::None)
    {
        registerFallbackIrInstruction(table,
                                      name,
                                      IrInstCategory::Compare,
                                      IrInstTier::HighLevel,
                                      static_cast<IrInstFlag>(static_cast<uint32_t>(IrInstFlag::SizeMatch) |
                                                              static_cast<uint32_t>(extraFlags)),
                                      { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                                        { IrOperandType::Register, "lhs", IrOperandDir::ArgIn },
                                        { IrOperandType::RegImm, "rhs", IrOperandDir::ArgIn } });
    };

    // Arithmetic
    binaryAlu("ADD", IrInstFlag::IsCommutative);
    binaryAlu("SUB");
    binaryAlu("MUL", IrInstFlag::IsCommutative);
    binaryAlu("IMUL",
              static_cast<IrInstFlag>(static_cast<uint32_t>(IrInstFlag::IsCommutative) |
                                      static_cast<uint32_t>(IrInstFlag::TreatAsSigned)));
    binaryAlu("DIV");
    binaryAlu("IDIV", IrInstFlag::TreatAsSigned);
    binaryAlu("SDIV", IrInstFlag::TreatAsSigned);
    binaryAlu("UDIV");
    binaryAlu("REM");
    binaryAlu("SREM", IrInstFlag::TreatAsSigned);
    binaryAlu("UREM");

    registerFallbackIrInstruction(table,
                                  "NEG",
                                  IrInstCategory::Arithmetic,
                                  IrInstTier::HighLevel,
                                  IrInstFlag::None,
                                  { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                                    { IrOperandType::Register, "src", IrOperandDir::ArgIn } });

    // Bitwise
    binaryBitwise("AND", IrInstFlag::IsCommutative);
    binaryBitwise("OR", IrInstFlag::IsCommutative);
    binaryBitwise("XOR", IrInstFlag::IsCommutative);
    registerFallbackIrInstruction(table,
                                  "NOT",
                                  IrInstCategory::Bitwise,
                                  IrInstTier::HighLevel,
                                  IrInstFlag::None,
                                  { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                                    { IrOperandType::Register, "src", IrOperandDir::ArgIn } });

    // Shifts
    shiftOp("SHL");
    shiftOp("SHR");
    shiftOp("SAR", IrInstFlag::TreatAsSigned);
    shiftOp("ROTL");
    shiftOp("ROTR");

    // Comparisons
    compareOp("CMP_EQ", IrInstFlag::IsCommutative);
    compareOp("CMP_NE", IrInstFlag::IsCommutative);
    compareOp("CMP_SLT", IrInstFlag::TreatAsSigned);
    compareOp("CMP_SLE", IrInstFlag::TreatAsSigned);
    compareOp("CMP_SGT", IrInstFlag::TreatAsSigned);
    compareOp("CMP_SGE", IrInstFlag::TreatAsSigned);
    compareOp("CMP_ULT");
    compareOp("CMP_ULE");
    compareOp("CMP_UGT");
    compareOp("CMP_UGE");

    // Data movement
    registerFallbackIrInstruction(table,
                                  "MOV",
                                  IrInstCategory::DataMovement,
                                  IrInstTier::HighLevel,
                                  IrInstFlag::None,
                                  { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                                    { IrOperandType::AnyValue, "src", IrOperandDir::ArgIn } });

    // Memory
    registerFallbackIrInstruction(table,
                                  "LOAD",
                                  IrInstCategory::Memory,
                                  IrInstTier::HighLevel,
                                  IrInstFlag::ReadsMemory,
                                  { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                                    { IrOperandType::AddressSource, "src", IrOperandDir::ArgIn } });

    registerFallbackIrInstruction(table,
                                  "STORE",
                                  IrInstCategory::Memory,
                                  IrInstTier::HighLevel,
                                  static_cast<IrInstFlag>(static_cast<uint32_t>(IrInstFlag::WritesMemory) |
                                                          static_cast<uint32_t>(IrInstFlag::HasSideEffect)),
                                  { { IrOperandType::AddressSource, "dst", IrOperandDir::ArgIn },
                                    { IrOperandType::AnyValue, "src", IrOperandDir::ArgIn } });

    registerFallbackIrInstruction(table,
                                  "ALLOC",
                                  IrInstCategory::Memory,
                                  IrInstTier::HighLevel,
                                  IrInstFlag::HasSideEffect,
                                  { { IrOperandType::Register, "dst", IrOperandDir::ArgOut } });

    // Control flow / system
    registerFallbackIrInstruction(table,
                                  "CALL",
                                  IrInstCategory::System,
                                  IrInstTier::HighLevel,
                                  static_cast<IrInstFlag>(static_cast<uint32_t>(IrInstFlag::IsCall) |
                                                          static_cast<uint32_t>(IrInstFlag::HasSideEffect)),
                                  { { IrOperandType::AnyValue, "callee", IrOperandDir::ArgIn } });

    registerFallbackIrInstruction(table,
                                  "RET",
                                  IrInstCategory::System,
                                  IrInstTier::HighLevel,
                                  static_cast<IrInstFlag>(static_cast<uint32_t>(IrInstFlag::IsReturn) |
                                                          static_cast<uint32_t>(IrInstFlag::IsTerminator)),
                                  { { IrOperandType::AnyValue, "val", IrOperandDir::ArgIn } });
}

// Walks up from the current directory looking for EzMir/instructions.irdf; empty when not found.
std::filesystem::path findInstructionsIrdf()
{
    std::filesystem::path cur = std::filesystem::current_path();
    for (int i = 0; i < 6; ++i)
    {
        auto cand = cur / "EzMir" / "instructions.irdf";
        std::error_code ec;
        if (std::filesystem::exists(cand, ec))
            return cand;
        if (cur.has_parent_path() && cur.parent_path() != cur)
            cur = cur.parent_path();
        else
            break;
    }
    return {};
}

// One file extension mapped to the dialect its contents declare.
struct ExtensionDialect
{
    std::string_view m_extension;  ///< Lower-case extension including the leading dot.
    LanguageDialect m_dialect;     ///< Dialect selected for that extension.
};

// Authoritative extension -> dialect list (drives detectDialect).
constexpr ExtensionDialect kExtensionDialects[] = {
    { ".tyf", LanguageDialect::TypeDef },       { ".irdf", LanguageDialect::IrInstDef },
    { ".lad", LanguageDialect::LegalizeAction }, { ".lrd", LanguageDialect::LegalizeRule },
    { ".idf", LanguageDialect::TargetInstDef },  { ".isf", LanguageDialect::InstructionSelect },
    { ".ezcc", LanguageDialect::CallingConv },   { ".ccd", LanguageDialect::CallingConv },
    { ".tdesc", LanguageDialect::TargetDesc },
};

// Metadata describing one generator: everything the driver needs besides construction itself.
struct GeneratorMeta
{
    GeneratorKind m_kind;              ///< Generator selector.
    LanguageDialect m_dialect;         ///< Dialect this generator consumes.
    std::string_view m_displayName;    ///< Class name reported by --dump-info.
    bool m_targetQualified;            ///< True when output names are prefixed with the target.
    std::string_view m_pairBaseName;   ///< Header/source base name (formatted with target when qualified).
    std::string_view m_singleFileName; ///< Header-only file name (formatted with target when qualified).
    bool m_headerAndSource;            ///< True for a .h/.cpp pair, false for a single header.
    std::string_view m_fallbackTarget; ///< Target fallback used when none is supplied.
};

// Single source of truth for generator identity, dialect, display name and output naming.
constexpr GeneratorMeta kGeneratorTable[] = {
    { GeneratorKind::TypeTable, LanguageDialect::TypeDef, "CppMirTypeTableGenerator", false, "MirTypeTable", "", true,
      "Target" },
    { GeneratorKind::Instructions, LanguageDialect::IrInstDef, "CppMirInstructionGenerator", false, "",
      "MirInstructionSetDefs.h", false, "Target" },
    { GeneratorKind::Legalizer, LanguageDialect::LegalizeAction, "CppLegalizerGenerator", true,
      "{}LegalizerActionTable", "", true, "Target" },
    { GeneratorKind::Rules, LanguageDialect::LegalizeRule, "CppLegalizeRuleGenerator", true, "{}LegalizerRules", "",
      true, "Target" },
    { GeneratorKind::TargetInstructions, LanguageDialect::TargetInstDef, "CppTargetInstructionGenerator", true,
      "{}TargetInstructionTable", "", true, "Target" },
    { GeneratorKind::TargetEncodings, LanguageDialect::TargetInstDef, "CppEncodingTableGenerator", true, "",
      "{}EncodingTable.h", false, "Target" },
    { GeneratorKind::InstructionSelector, LanguageDialect::InstructionSelect, "CppInstructionSelectorGenerator", true,
      "{}InstructionSelector", "", true, "Target" },
    { GeneratorKind::CallingConv, LanguageDialect::CallingConv, "CppCallingConvGenerator", true,
      "{}CallingConvDesc", "", true, "CallingConv" },
    { GeneratorKind::RegisterInfo, LanguageDialect::TargetDesc, "CppRegisterInfoGenerator", true, "",
      "{}RegisterInfo.h", false, "Target" },
    { GeneratorKind::TargetDesc, LanguageDialect::TargetDesc, "CppTargetDescGenerator", true, "{}TargetDesc", "", true,
      "Target" },
};

// Looks up the metadata for genKind; returns nullptr for Auto/unknown kinds.
const GeneratorMeta *findGeneratorMeta(GeneratorKind genKind)
{
    for (const auto &meta : kGeneratorTable)
    {
        if (meta.m_kind == genKind)
        {
            return &meta;
        }
    }
    return nullptr;
}

} // namespace

// Stores the resolved CLI options; concrete work happens in run().
Driver::Driver(CliOptions options) : m_options(std::move(options)) {}

// Resolves the input dialect from an explicit override, the file extension, or the requested generator.
LanguageDialect Driver::detectDialect(const std::filesystem::path &filePath) const
{
    if (m_options.dialect != LanguageDialect::Auto)
    {
        return m_options.dialect;
    }

    const std::string ext = NormalizeKey(filePath.extension().string());
    for (const auto &entry : kExtensionDialects)
    {
        if (entry.m_extension == ext)
        {
            return entry.m_dialect;
        }
    }

    // No known extension: fall back to the dialect of an explicitly requested generator.
    if (const GeneratorMeta *meta = findGeneratorMeta(m_options.generator))
    {
        return meta->m_dialect;
    }

    return LanguageDialect::Auto;
}

// Maps a detected dialect to the generator that consumes it, honoring an explicit --generator.
GeneratorKind Driver::resolveGeneratorKind(LanguageDialect dialect) const
{
    if (m_options.generator != GeneratorKind::Auto)
    {
        return m_options.generator;
    }

    for (const auto &meta : kGeneratorTable)
    {
        if (meta.m_dialect == dialect)
        {
            return meta.m_kind;
        }
    }

    return GeneratorKind::Auto;
}

// Resolves the target identifier (explicit, then input stem, then fallback) sanitized for C++.
std::string Driver::resolveTargetName(std::string_view fallback) const
{
    std::string target = m_options.targetName;
    if (target.empty() && !m_options.inputFilePath.empty())
    {
        target = std::filesystem::path(m_options.inputFilePath).stem().string();
    }
    if (target.empty())
    {
        target = std::string(fallback);
    }
    return SanitizeCppIdentifier(target, fallback);
}

// Predicts the artifacts a generator will write so the driver can report and dry-run them.
std::vector<OutputFileInfo> Driver::computeExpectedOutputs(GeneratorKind genKind,
                                                           const std::filesystem::path &outDir) const
{
    std::vector<OutputFileInfo> outputs;

    const GeneratorMeta *meta = findGeneratorMeta(genKind);
    if (!meta)
    {
        return outputs;
    }

    const bool emitHeader = !m_options.sourceOnly || m_options.headerOnly;
    const bool emitSource = !m_options.headerOnly || m_options.sourceOnly;

    // Appends a header/source pair, formatting the base name with the resolved target when qualified.
    auto appendPair = [&](std::string_view baseNameFormat)
    {
        std::string baseName(baseNameFormat);
        if (meta->m_targetQualified)
        {
            const std::string target = resolveTargetName(meta->m_fallbackTarget);
            baseName = std::vformat(baseNameFormat, std::make_format_args(target));
        }

        auto paths = CodeGenerators::CodeGenerator::ResolveHeaderAndSourcePaths(outDir, baseName);
        if (emitHeader)
        {
            outputs.push_back(
                    { .role = "header", .path = paths.m_headerPath, .exists = std::filesystem::exists(paths.m_headerPath) });
        }
        if (emitSource)
        {
            outputs.push_back(
                    { .role = "source", .path = paths.m_sourcePath, .exists = std::filesystem::exists(paths.m_sourcePath) });
        }
    };

    // Appends a single header artifact, formatting the file name with the resolved target when qualified.
    auto appendSingle = [&](std::string_view fileNameFormat)
    {
        if (!emitHeader)
        {
            return;
        }

        std::string fileName(fileNameFormat);
        if (meta->m_targetQualified)
        {
            const std::string target = resolveTargetName(meta->m_fallbackTarget);
            fileName = std::vformat(fileNameFormat, std::make_format_args(target));
        }

        const auto filePath = CodeGenerators::CodeGenerator::ResolveSingleFilePath(outDir, fileName);
        outputs.push_back({ .role = "header", .path = filePath, .exists = std::filesystem::exists(filePath) });
    };

    if (meta->m_headerAndSource)
    {
        appendPair(meta->m_pairBaseName);

        // A .lad input may be accompanied by a .lrd file, which the legalizer run also emits.
        if (meta->m_kind == GeneratorKind::Legalizer)
        {
            bool hasRules = !m_options.rulesFilePath.empty();
            if (!hasRules && !m_options.inputFilePath.empty())
            {
                auto adj = std::filesystem::path(m_options.inputFilePath);
                adj.replace_extension(".lrd");
                std::error_code ec;
                if (std::filesystem::exists(adj, ec))
                {
                    hasRules = true;
                }
            }
            if (hasRules)
            {
                appendPair("{}LegalizerRules");
            }
        }
    }
    else
    {
        appendSingle(meta->m_singleFileName);
    }

    return outputs;
}

// Executes the full parse -> sema -> dump -> generate pipeline for the configured options.
DriverResult Driver::run()
{
    DriverResult result;
    std::filesystem::path inputPath(m_options.inputFilePath);

    // Fail fast when the primary input cannot be found on disk.
    if (!std::filesystem::exists(inputPath))
    {
        result.success = false;
        result.errorMessage = std::format("Input file '{}' does not exist.", inputPath.string());
        return result;
    }

    // Arena and diagnostics live for the duration of the run.
    std::pmr::monotonic_buffer_resource arena;
    DiagnosticCollector diagCollector;

    // Verbose mode enables the most granular diagnostics.
    if (m_options.verbose)
    {
        diagCollector.enableDiag(Diag_Trace);
        diagCollector.enableDiag(Diag_Debug);
    }

    // Resolve included files relative to the working directory plus any -I paths.
    SourceManager sourceManager(std::filesystem::current_path(), &arena);
    for (const auto &inc : m_options.includeDirs)
    {
        sourceManager.addIncludePath(inc);
    }

    // The tracker is always attached so the driver can abort on any error diagnostic.
    ErrorTrackingListener errorTracker;
    diagCollector.addListener(&errorTracker);

    // Human-readable diagnostics are suppressed under --quiet.
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
                                          "Please use a known extension (.tyf, .irdf, .lad, .lrd, .idf, .isf, .ezcc, "
                                          ".ccd, .reg, .tdesc) "
                                          "or specify --generator / --emit-* option.",
                                          inputPath.string());
        return result;
    }

    GeneratorKind genKind = resolveGeneratorKind(dialect);
    std::vector<OutputFileInfo> expectedOutputs = computeExpectedOutputs(genKind, m_options.outputPath);

    // Track write timestamps of expected output files before generation, keyed by path (OPT-07).
    std::unordered_map<std::filesystem::path, std::filesystem::file_time_type> preModTimes;
    for (const auto &out : expectedOutputs)
    {
        std::error_code ec;
        if (std::filesystem::exists(out.path, ec))
        {
            preModTimes[out.path] = std::filesystem::last_write_time(out.path, ec);
        }
    }

    // Parsing and semantic state shared by all dialects.
    ParseContext parseCtx(&diagCollector, &sourceManager, sourceId, &arena);
    SymbolTable symbolTable(&arena);
    size_t constructCount = 0;
    bool hasLoadedRules = false;
    std::optional<DSL::Ast::CallingConvDef::CallingConventionDefFile> ccAst;
    std::optional<DSL::Ast::InstructionSelectDef::InstructionSelectFile> isAst;
    std::optional<DSL::Ast::TargetDesc::TargetDescFile> tdAst;

    // Multi-dialect prelude & dependency ingestion
    if (dialect == LanguageDialect::LegalizeRule || dialect == LanguageDialect::LegalizeAction)
    {
        // 1. Ingest Types
        if (!m_options.typesFilePath.empty())
        {
            std::filesystem::path typesPath(m_options.typesFilePath);
            auto typesSourceId = sourceManager.loadFile(typesPath);
            if (!typesSourceId.has_value())
            {
                result.success = false;
                result.errorMessage = std::format("Failed to load types file '{}'.", typesPath.string());
                return result;
            }
            ParseContext typesParseCtx(&diagCollector, &sourceManager, *typesSourceId, &arena);
            auto typesAst = typesParseCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
            if (!typesAst.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = std::format("Syntax parsing failed for types file '{}'.", typesPath.string());
                return result;
            }
            TypePass typePass;
            if (!typePass.run(&diagCollector, &symbolTable, &typesAst.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = std::format("Semantic analysis failed for types file '{}'.", typesPath.string());
                return result;
            }
        }
        else
        {
            initializeStandardTypes(symbolTable);
        }

        // 2. Ingest IR Instructions
        if (!m_options.instructionsFilePath.empty())
        {
            std::filesystem::path instPath(m_options.instructionsFilePath);
            auto instSourceId = sourceManager.loadFile(instPath);
            if (!instSourceId.has_value())
            {
                result.success = false;
                result.errorMessage = std::format("Failed to load instructions file '{}'.", instPath.string());
                return result;
            }
            ParseContext instParseCtx(&diagCollector, &sourceManager, *instSourceId, &arena);
            auto instAst =
                    instParseCtx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
            if (!instAst.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage =
                        std::format("Syntax parsing failed for instructions file '{}'.", instPath.string());
                return result;
            }
            IrInstructionPass instPass;
            if (!instPass.run(&diagCollector, &symbolTable, &instAst.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage =
                        std::format("Semantic analysis failed for instructions file '{}'.", instPath.string());
                return result;
            }
        }
        else
        {
            auto autoIrdf = findInstructionsIrdf();
            if (!autoIrdf.empty())
            {
                auto instSourceId = sourceManager.loadFile(autoIrdf);
                if (instSourceId.has_value())
                {
                    ParseContext instParseCtx(&diagCollector, &sourceManager, *instSourceId, &arena);
                    auto instAst =
                            instParseCtx
                                    .parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
                    if (instAst.has_value() && !errorTracker.hasErrors())
                    {
                        IrInstructionPass instPass;
                        instPass.run(&diagCollector, &symbolTable, &instAst.value());
                    }
                }
            }
            initializeStandardIrInstructions(symbolTable);
        }

        // 3. Paired discovery of rewrite rules for LegalizeAction (.lad)
        if (dialect == LanguageDialect::LegalizeAction)
        {
            std::filesystem::path rulesPath;
            if (!m_options.rulesFilePath.empty())
            {
                rulesPath = m_options.rulesFilePath;
            }
            else
            {
                auto adj = inputPath;
                adj.replace_extension(".lrd");
                std::error_code ec;
                if (std::filesystem::exists(adj, ec))
                {
                    rulesPath = adj;
                }
            }

            if (!rulesPath.empty())
            {
                auto rulesSourceId = sourceManager.loadFile(rulesPath);
                if (!rulesSourceId.has_value())
                {
                    result.success = false;
                    result.errorMessage = std::format("Failed to load companion rules file '{}'.", rulesPath.string());
                    return result;
                }
                ParseContext rulesParseCtx(&diagCollector, &sourceManager, *rulesSourceId, &arena);
                auto rulesAst = rulesParseCtx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRuleFile,
                                                    DSL::Ast::LegalizeRuleDef::LegalizeRuleFile>();
                if (!rulesAst.has_value() || errorTracker.hasErrors())
                {
                    result.success = false;
                    result.errorMessage =
                            std::format("Syntax parsing failed for companion rules file '{}'.", rulesPath.string());
                    return result;
                }

                if (!LegalizeRulePass::run(&diagCollector, &symbolTable, &rulesAst.value()) || errorTracker.hasErrors())
                {
                    result.success = false;
                    result.errorMessage =
                            std::format("Semantic analysis failed for companion rules file '{}'.", rulesPath.string());
                    return result;
                }
                hasLoadedRules = true;
            }
        }
    }

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

        case LanguageDialect::TargetInstDef:
        {
            auto ast = parseCtx.parse<DSL::Parser::TargetInstDef::TargetInstFile,
                                      DSL::Ast::TargetInstDef::TargetInstFile>();
            if (!ast.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Syntax parsing failed for Target Instruction Definition file.";
                return result;
            }

            constructCount = ast->m_instructions.size();

            if (!TargetInstPass::run(&diagCollector, &symbolTable, &ast.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Semantic analysis failed for Target Instruction Definition file.";
                return result;
            }
            break;
        }

        case LanguageDialect::InstructionSelect:
        {
            isAst = parseCtx.parse<DSL::Parser::InstructionSelectDef::InstructionSelectFile,
                                   DSL::Ast::InstructionSelectDef::InstructionSelectFile>();
            if (!isAst.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Syntax parsing failed for Instruction Selection Definition file.";
                return result;
            }

            constructCount = isAst->m_patterns.size();

            if (!InstructionSelectPass::run(&diagCollector, &symbolTable, &isAst.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Semantic analysis failed for Instruction Selection Definition file.";
                return result;
            }
            break;
        }

        case LanguageDialect::CallingConv:
        {
            ccAst = parseCtx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile,
                                   DSL::Ast::CallingConvDef::CallingConventionDefFile>();
            if (!ccAst.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Syntax parsing failed for Calling Convention Definition file.";
                return result;
            }

            constructCount = 1;

            if (m_options.dumpAst)
            {
                InfoDumper::dumpCallingConvAst(*ccAst, m_options.format, std::cout);
            }

            if (!CallingConvPass::run(&diagCollector, &symbolTable, &ccAst.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Semantic analysis failed for Calling Convention Definition file.";
                return result;
            }
            break;
        }

        case LanguageDialect::TargetDesc:
        {
            tdAst = parseCtx.parse<DSL::Parser::TargetDesc::TargetDescFileParser,
                                   DSL::Ast::TargetDesc::TargetDescFile>();
            if (!tdAst.has_value() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Syntax parsing failed for Target Descriptor file.";
                return result;
            }

            constructCount = 1;

            if (m_options.dumpAst)
            {
                InfoDumper::dumpTargetDescAst(*tdAst, m_options.format, std::cout);
            }

            TargetDescPass pass;
            if (!pass.run(&diagCollector, &symbolTable, &tdAst.value()) || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Semantic analysis failed for Target Descriptor file.";
                return result;
            }
            break;
        }

        default:
            break;
    }

    // Optional inspection dumps, all sent to stdout in the requested format.
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
        // Parse the primary file and run the dialect's semantic pass into the shared symbol table.
        switch (dialect)
        {
            case LanguageDialect::TypeDef:
                gInfo.dialectName = "TypeDef (.tyf)";
                break;
            case LanguageDialect::IrInstDef:
                gInfo.dialectName = "IrInstDef (.irdf)";
                break;
            case LanguageDialect::LegalizeAction:
                gInfo.dialectName = "LegalizeAction (.lad)";
                break;
            case LanguageDialect::LegalizeRule:
                gInfo.dialectName = "LegalizeRule (.lrd)";
                break;
            case LanguageDialect::TargetInstDef:
                gInfo.dialectName = "TargetInstDef (.idf)";
                break;
            case LanguageDialect::InstructionSelect:
                gInfo.dialectName = "InstructionSelect (.isf)";
                break;
            case LanguageDialect::CallingConv:
                gInfo.dialectName = "CallingConv (.ezcc, .ccd)";
                break;
            case LanguageDialect::TargetDesc:
                gInfo.dialectName = "TargetDesc (.tdesc)";
                break;
            default:
                gInfo.dialectName = "Unknown";
                break;
        }
        gInfo.constructCount = constructCount;

        if (const GeneratorMeta *meta = findGeneratorMeta(genKind))
        {
            gInfo.generatorName = std::string(meta->m_displayName);
        }
        else
        {
            gInfo.generatorName = "None";
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

    // Check-only and dry-run stop after analysis and inspection without writing files.
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
    else if (genKind == GeneratorKind::Legalizer)
    {
        using namespace CodeGenerators;
        const std::string target = resolveTargetName("Target");

        CppLegalizerGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, target,
                                        m_options.namespaceRoot);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during LegalizerActionTable synthesis.";
            return result;
        }

        if (hasLoadedRules)
        {
            CppLegalizeRuleGenerator ruleGen(&diagCollector, &symbolTable, m_options.outputPath, target,
                                             m_options.namespaceRoot);
            if (!ruleGen.run() || errorTracker.hasErrors())
            {
                result.success = false;
                result.errorMessage = "Code generation failed during companion LegalizerRules synthesis.";
                return result;
            }
        }
    }
    else if (genKind == GeneratorKind::Rules)
    {
        using namespace CodeGenerators;
        const std::string target = resolveTargetName("Target");

        CppLegalizeRuleGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, target,
                                           m_options.namespaceRoot);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during LegalizerRules synthesis.";
            return result;
        }
    }
    else if (genKind == GeneratorKind::TargetInstructions)
    {
        using namespace CodeGenerators;
        const std::string target = resolveTargetName("Target");

        CppTargetInstructionGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, target,
                                                m_options.namespaceRoot);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during TargetInstructionTable synthesis.";
            return result;
        }
    }
    else if (genKind == GeneratorKind::TargetEncodings)
    {
        using namespace CodeGenerators;
        const std::string target = resolveTargetName("Target");

        CppEncodingTableGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, target);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during TargetEncodingTable synthesis.";
            return result;
        }
    }
    else if (genKind == GeneratorKind::InstructionSelector)
    {
        using namespace CodeGenerators;
        const std::string target = resolveTargetName("Target");

        CppInstructionSelectorGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, target,
                                                  m_options.namespaceRoot);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during InstructionSelector synthesis.";
            return result;
        }
    }
    else if (genKind == GeneratorKind::CallingConv)
    {
        using namespace CodeGenerators;
        const std::string target = resolveTargetName("CallingConv");

        CppCallingConvGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, target);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during CallingConvDesc synthesis.";
            return result;
        }
    }
    else if (genKind == GeneratorKind::RegisterInfo)
    {
        using namespace CodeGenerators;
        const std::string target = resolveTargetName("Target");

        CppRegisterInfoGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, target,
                                           m_options.namespaceRoot);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during RegisterInfo synthesis.";
            return result;
        }
    }
    else if (genKind == GeneratorKind::TargetDesc)
    {
        using namespace CodeGenerators;
        const std::string target = resolveTargetName("Target");

        CppTargetDescGenerator generator(&diagCollector, &symbolTable, m_options.outputPath, target,
                                         m_options.namespaceRoot);
        if (!generator.run() || errorTracker.hasErrors())
        {
            result.success = false;
            result.errorMessage = "Code generation failed during TargetDesc synthesis.";
            return result;
        }
    }
    else
    {
        // No generator available
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
            auto it = preModTimes.find(out.path);
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

    // Report the synthesized/up-to-date artifacts unless silenced by --quiet.
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
