#ifndef EZDSL_CPP_LEGALIZE_RULE_GENERATOR_H
#define EZDSL_CPP_LEGALIZE_RULE_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes the target-specific LegalizerRules header and implementation files
 * (<Target>LegalizerRules.h and .cpp) from EzDSL .lrd semantic definitions.
 * Emits pattern matcher and rewriter functions, predicate guard forward declarations,
 * custom transform invocations, and instruction replacement sequences.
 */
class CppLegalizeRuleGenerator : public CodeGenerator
{
  public:
    /**
     * Constructs a generator that synthesizes legalizer rule matchers/rewriters for the given target.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed .lrd rule definitions.
     * @param outPath Destination file or directory for the generated artifacts.
     * @param targetName Target identifier substituted into generated class names.
     */
    CppLegalizeRuleGenerator(DiagnosticCollector *collector,
                             SymbolTable *table,
                             std::filesystem::path outPath,
                             std::string targetName = "Target");

    /** Generates the rule header/source pair; returns false if validation or emission fails. */
    bool run() override;

    /** Returns the target identifier used to name generated classes. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to name generated classes. */
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

    /** Emits the rule matcher/rewriter declarations and predicate forward declarations into the header. */
    void emitHeader(CppSourceEmitter &emitter) const;

    /** Emits the matcher and rewriter definitions plus instruction replacement sequences into the source. */
    void emitSource(CppSourceEmitter &emitter) const;

  private:
    /** Target identifier substituted into generated class and include names. */
    std::string m_targetName;
};

/**
 * Convenience entry point for generating the target LegalizerRules.
 */
extern bool GenerateLegalizerRules(DiagnosticCollector *collector,
                                   SymbolTable *table,
                                   std::filesystem::path outPath,
                                   std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_LEGALIZE_RULE_GENERATOR_H
