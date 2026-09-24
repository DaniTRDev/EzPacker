#ifndef EZDSL_CLI_INFO_DUMPER_H
#define EZDSL_CLI_INFO_DUMPER_H

#include "Cli/CommandLineOptions.h"
#include "EzDslCliCommon.h"

#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"

// Forward declarations
namespace DSL::Ast::TypeDef
{
struct TypeDefFile;
struct TypeDescriptor;
} // namespace DSL::Ast::TypeDef

namespace DSL::Ast::IrInstDef
{
struct IrInstDefFile;
struct IrInstDecl;
} // namespace DSL::Ast::IrInstDef

namespace DSL::Ast::LegalizeActionDef
{
struct LegalizeActionFile;
}

namespace DSL::Ast::LegalizeRuleDef
{
struct LegalizeRuleFile;
}

namespace DSL::Ast::CallingConvDef
{
struct CallingConventionDefFile;
}

namespace DSL::Ast::TargetDesc
{
struct TargetDescFile;
}

class SymbolTable;

namespace Cli
{

/**
 * Describes one generated (or expected) output artifact for reporting.
 */
struct OutputFileInfo
{
    std::string role;           // "header", "source", etc.
    std::filesystem::path path; ///< Location of the artifact.
    bool exists{ false };       ///< Whether the file currently exists on disk.
};

/**
 * Aggregated, generator-independent summary of a single input file, used by the --dump-info output.
 */
struct GeneralFileInfo
{
    std::filesystem::path inputPath;                  ///< Input file being described.
    size_t fileSizeBytes{ 0 };                        ///< Size of the input in bytes.
    LanguageDialect dialect{ LanguageDialect::Auto }; ///< Dialect detected for the input.
    std::string dialectName;                          ///< Display name of the detected dialect.
    size_t constructCount{ 0 };                       ///< Number of top-level DSL constructs parsed.
    std::string generatorName;                        ///< Name of the generator selected for the input.
    std::string workingMode;               ///< Mode string (e.g. Header/Source/Full) for the selected generator.
    std::filesystem::path outputDirectory; ///< Directory receiving generated artifacts.
    std::vector<OutputFileInfo> outputs;   ///< Expected output artifacts.
};

/**
 * Stateless formatter that serializes parser/sema results in either text or JSON form.
 * All entry points are static and write directly to the supplied stream.
 */
class InfoDumper
{
  public:
    /** Writes the general input/output summary in the requested format. */
    static void dumpGeneralInfo(const GeneralFileInfo &info, OutputFormat format, std::ostream &os);

    /** Writes the list of expected output files in the requested format. */
    static void dumpOutputFiles(const std::vector<OutputFileInfo> &files, OutputFormat format, std::ostream &os);

    /** Writes a parsed .tyf AST in the requested format. */
    static void dumpTypeDefAst(const DSL::Ast::TypeDef::TypeDefFile &file, OutputFormat format, std::ostream &os);

    /** Writes a parsed .irdf AST in the requested format. */
    static void dumpIrInstDefAst(const DSL::Ast::IrInstDef::IrInstDefFile &file, OutputFormat format, std::ostream &os);

    /** Writes a parsed .lad AST in the requested format. */
    static void dumpLegalizeActionAst(const DSL::Ast::LegalizeActionDef::LegalizeActionFile &file,
                                      OutputFormat format,
                                      std::ostream &os);

    /** Writes a parsed .lrd AST in the requested format. */
    static void
    dumpLegalizeRuleAst(const DSL::Ast::LegalizeRuleDef::LegalizeRuleFile &file, OutputFormat format, std::ostream &os);

    /** Writes a parsed .ezcc/.ccd AST in the requested format. */
    static void dumpCallingConvAst(const DSL::Ast::CallingConvDef::CallingConventionDefFile &file,
                                   OutputFormat format,
                                   std::ostream &os);

    /** Writes a parsed .tdesc AST in the requested format. */
    static void
    dumpTargetDescAst(const DSL::Ast::TargetDesc::TargetDescFile &file, OutputFormat format, std::ostream &os);

    /** Writes the semantic symbol table in the requested format. */
    static void dumpSymbols(const SymbolTable &symbolTable, OutputFormat format, std::ostream &os);

    /** Escapes characters that are illegal in JSON string literals. */
    static std::string escapeJson(std::string_view str);

  private:
    /** Converts an AST type-kind enum value to its display string. */
    static std::string_view typeKindToString(DSL::Ast::TypeDef::TypeKind kind);

    /** Converts an IR instruction category enum value to its display string. */
    static std::string_view irCategoryToString(DSL::Ast::IrInstDef::IrInstCategory cat);

    /** Converts an IR instruction tier enum value to its display string. */
    static std::string_view irTierToString(DSL::Ast::IrInstDef::IrInstTier tier);

    /** Converts an IR operand direction enum value to its display string. */
    static std::string_view irOperandDirToString(DSL::Ast::IrInstDef::IrOperandDir dir);

    /** Converts an IR operand type bitmask into a list of individual type names. */
    static std::string irOperandTypeToString(uint16_t typeMask);

    /** Converts an IR instruction flag bitmask into a list of individual flag names. */
    static std::vector<std::string> irFlagsToStrings(uint32_t flagMask);
};

} // namespace Cli

#endif // EZDSL_CLI_INFO_DUMPER_H
