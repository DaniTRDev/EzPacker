#ifndef EZDSL_CPP_MIR_TYPE_TABLE_GENERATOR_H
#define EZDSL_CPP_MIR_TYPE_TABLE_GENERATOR_H

#include "Ast/TypeDefLangAst.h"
#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the code generation pass.
 */
enum class MirTypeTableGenWorkingMode : uint8_t
{
    Header = 1,            // Generates ONLY the header file (MirTypeTable.h).
    Source = 1 << 1,       // Generates ONLY the translation unit (MirTypeTable.cpp).
    Full = Header | Source // Generates both the header and the source files.
};

constexpr MirTypeTableGenWorkingMode operator|(MirTypeTableGenWorkingMode a, MirTypeTableGenWorkingMode b) noexcept
{
    return static_cast<MirTypeTableGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(MirTypeTableGenWorkingMode a, MirTypeTableGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes the EzMir TypeTable C++ class hierarchy and initialization logic from EzDSL metadata.
 */
class CppMirTypeTableGenerator : public CodeGenerator
{
  public:
    struct TypeEntry
    {
        std::string m_name;
        uint32_t m_bitWidth{ 0 };
        uint32_t m_alignment{ 0 };
        DSL::Ast::TypeDef::TypeKind m_kind{ DSL::Ast::TypeDef::TypeKind::Integer };
        std::string m_fieldName;
        std::string m_getterName;
        uint8_t m_compactId{ 0 };
    };

  public:
    CppMirTypeTableGenerator(DiagnosticCollector *collector,
                             SymbolTable *table,
                             std::filesystem::path outPath,
                             MirTypeTableGenWorkingMode mode = MirTypeTableGenWorkingMode::Full);

    bool run() override;

    void emitHeader(CppSourceEmitter &emitter, const std::vector<TypeEntry> &types) const;
    void emitSource(CppSourceEmitter &emitter, const std::vector<TypeEntry> &types) const;

    std::vector<TypeEntry> collectTypes() const;

  private:
    MirTypeTableGenWorkingMode m_mode;
};

/**
 * Convenience entry point maintaining backward compatibility with the existing API.
 */
extern bool GenerateMirTypeTable(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 std::filesystem::path outPath,
                                 MirTypeTableGenWorkingMode mode = MirTypeTableGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_MIR_TYPE_TABLE_GENERATOR_H