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
    /**
     * Constructs a generator that synthesizes the legalizer action table for the given target.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed .lad action definitions.
     * @param outPath Destination file or directory for the generated artifacts.
     * @param targetName Target identifier substituted into generated class names.
     */
    CppLegalizerGenerator(DiagnosticCollector *collector,
                          SymbolTable *table,
                          std::filesystem::path outPath,
                          std::string targetName = "Target");

    /** Generates the legalizer action table header/source pair; returns false if validation or emission fails. */
    bool run() override;

    /** Returns the target identifier used to name generated classes. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to name generated classes. */
    void setTargetName(std::string targetName) { m_targetName = SanitizeCppIdentifier(targetName, "Target"); }

    /** Emits the legalizer action table declarations and dense matrix layout into the header. */
    void emitHeader(CppSourceEmitter &emitter) const;

    /** Emits the action table definition and lowering dispatch tables into the source. */
    void emitSource(CppSourceEmitter &emitter) const;

  private:
    /** Target identifier substituted into generated class and include names. */
    std::string m_targetName;
};

} // namespace CodeGenerators

#endif // EZDSL_CPP_LEGALIZER_GENERATOR_H
