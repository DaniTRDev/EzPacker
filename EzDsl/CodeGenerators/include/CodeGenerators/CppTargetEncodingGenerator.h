#ifndef EZDSL_CPP_TARGET_ENCODING_GENERATOR_H
#define EZDSL_CPP_TARGET_ENCODING_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes the target instruction encoding table header
 * (<Target>EncodingTable.h) from EzDSL .idf ENCODING blocks.
 *
 * The emitted table lives in EzCodeEmitter::TableGen::<Target> and is consumed by the
 * generic, target-agnostic TableGen::InstructionEncoder at emission time. It is
 * header-only so EzTriple, EzCodeEmitter and downstream users can include it without an
 * extra link dependency.
 */
class CppTargetEncodingGenerator : public CodeGenerator
{
  public:
    /**
     * Constructs a generator that synthesizes the target instruction encoding table.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed .idf ENCODING blocks.
     * @param outPath Destination file or directory for the generated header.
     * @param targetName Target identifier substituted into generated table names.
     */
    CppTargetEncodingGenerator(DiagnosticCollector *collector,
                               SymbolTable *table,
                               std::filesystem::path outPath,
                               std::string targetName = "Target");

    /** Generates the encoding table header; returns false if validation or emission fails. */
    bool run() override;

    /** Emits the header-only encoding table consumed by the generic InstructionEncoder. */
    void emitHeader(CppSourceEmitter &emitter) const;

    /** Returns the target identifier used to name generated tables. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to name generated tables. */
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

  private:
    /** Target identifier substituted into generated table and include names. */
    std::string m_targetName;
};

/**
 * Convenience entry point for generating the target encoding table.
 */
extern bool GenerateTargetEncodingTable(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        std::filesystem::path outPath,
                                        std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_TARGET_ENCODING_GENERATOR_H
