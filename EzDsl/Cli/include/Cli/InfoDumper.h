#ifndef EZDSL_CLI_INFO_DUMPER_H
#define EZDSL_CLI_INFO_DUMPER_H

#include "Cli/CommandLineOptions.h"
#include "EzDslCliCommon.h"

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

class SymbolTable;

namespace Cli
{

struct OutputFileInfo
{
    std::string role; // "header", "source", etc.
    std::filesystem::path path;
    bool exists{ false };
};

struct GeneralFileInfo
{
    std::filesystem::path inputPath;
    size_t fileSizeBytes{ 0 };
    LanguageDialect dialect{ LanguageDialect::Auto };
    std::string dialectName;
    size_t constructCount{ 0 };
    std::string generatorName;
    std::string workingMode;
    std::filesystem::path outputDirectory;
    std::vector<OutputFileInfo> outputs;
};

class InfoDumper
{
  public:
    static void dumpGeneralInfo(const GeneralFileInfo &info, OutputFormat format, std::ostream &os);

    static void dumpOutputFiles(const std::vector<OutputFileInfo> &files, OutputFormat format, std::ostream &os);

    static void dumpTypeDefAst(const DSL::Ast::TypeDef::TypeDefFile &file, OutputFormat format, std::ostream &os);

    static void dumpIrInstDefAst(const DSL::Ast::IrInstDef::IrInstDefFile &file, OutputFormat format, std::ostream &os);

    static void dumpLegalizeActionAst(const DSL::Ast::LegalizeActionDef::LegalizeActionFile &file,
                                     OutputFormat format,
                                     std::ostream &os);

    static void dumpLegalizeRuleAst(const DSL::Ast::LegalizeRuleDef::LegalizeRuleFile &file,
                                   OutputFormat format,
                                   std::ostream &os);

    static void dumpSymbols(const SymbolTable &symbolTable, OutputFormat format, std::ostream &os);

    static std::string escapeJson(std::string_view str);

  private:
    static std::string typeKindToString(int kind);
    static std::string irCategoryToString(int cat);
    static std::string irTierToString(int tier);
    static std::string irOperandDirToString(int dir);
    static std::string irOperandTypeToString(uint16_t typeMask);
    static std::vector<std::string> irFlagsToStrings(uint32_t flagMask);
};

} // namespace Cli

#endif // EZDSL_CLI_INFO_DUMPER_H
