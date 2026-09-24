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

/** Tests whether the Header/Source flag(s) in b are present in a. */
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
    /**
     * Flattened description of a single primitive/derived type as read from the .tyf symbol table.
     * Captures the ABI properties and the generated accessor/compact-id mapping for the type.
     */
    struct TypeEntry
    {
        std::string m_name;        ///< DSL type name (used as the C++ type enum name).
        uint32_t m_bitWidth{ 0 };  ///< Size of the type in bits.
        uint32_t m_alignment{ 0 }; ///< Required ABI alignment in bits.
        DSL::Ast::TypeDef::TypeKind m_kind{ DSL::Ast::TypeDef::TypeKind::Integer }; ///< Classification of the type.
        std::string m_fieldName;  ///< Name of the generated field accessor in the table.
        std::string m_getterName; ///< Name of the generated lookup function.
        uint8_t m_compactId{ 0 }; ///< Dense index assigned for compact type-id encoding.
    };

  public:
    /**
     * Constructs a generator that emits the EzMir type table from the parsed .tyf metadata.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed type definitions.
     * @param outPath Destination file or directory for the generated artifacts.
     * @param mode Selects whether to emit the header, the source, or both.
     */
    CppMirTypeTableGenerator(DiagnosticCollector *collector,
                             SymbolTable *table,
                             std::filesystem::path outPath,
                             MirTypeTableGenWorkingMode mode = MirTypeTableGenWorkingMode::Full);

    /** Generates the requested type table artifact(s); returns false if validation or emission fails. */
    bool run() override;

    /** Emits the type enum, property accessors and lookup declarations into the header. */
    void emitHeader(CppSourceEmitter &emitter, const std::vector<TypeEntry> &types) const;

    /** Emits the type table storage and initialization routine into the source. */
    void emitSource(CppSourceEmitter &emitter, const std::vector<TypeEntry> &types) const;

    /** Collects and flattens all type definitions from the symbol table into TypeEntry records. */
    std::vector<TypeEntry> collectTypes() const;

  private:
    /** Selects which artifacts run() is allowed to emit. */
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