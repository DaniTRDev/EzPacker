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
    CppTargetEncodingGenerator(DiagnosticCollector *collector,
                               SymbolTable *table,
                               std::filesystem::path outPath,
                               std::string targetName = "Target");

    bool run() override;

    void emitHeader(CppSourceEmitter &emitter) const;

    const std::string &getTargetName() const noexcept { return m_targetName; }
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

  private:
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
