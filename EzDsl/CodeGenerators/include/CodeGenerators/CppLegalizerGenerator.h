#ifndef EZDSL_CPP_LEGALIZER_GENERATOR_H
#define EZDSL_CPP_LEGALIZER_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes the target-specific LegalizerActionTable header and implementation files
 * (<Target>LegalizerActionTable.h and .cpp) from EzDSL .lad semantic definitions.
 * Emits an O(1) dense 2D primary matrix, sparse rule matchers, libcall tables, and lowering dispatchers.
 */
class CppLegalizerGenerator : public CodeGenerator
{
  public:
    CppLegalizerGenerator(DiagnosticCollector *collector,
                          SymbolTable *table,
                          std::filesystem::path outPath,
                          std::string targetName = "Target");

    bool run() override;

    const std::string &getTargetName() const noexcept { return m_targetName; }
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

    void emitHeader(CppSourceEmitter &emitter) const;
    void emitSource(CppSourceEmitter &emitter) const;

  private:
    std::string m_targetName;
};

/**
 * Convenience entry point for generating the target LegalizerActionTable.
 */
extern bool GenerateLegalizerActionTable(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         std::filesystem::path outPath,
                                         std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_LEGALIZER_GENERATOR_H
