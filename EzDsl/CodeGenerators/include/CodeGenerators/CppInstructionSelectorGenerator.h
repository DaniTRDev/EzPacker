#ifndef EZDSL_CPP_INSTRUCTION_SELECTOR_GENERATOR_H
#define EZDSL_CPP_INSTRUCTION_SELECTOR_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes target-specific instruction selector engine header and implementation files
 * (<Target>InstructionSelector.h and .cpp) from EzDSL .isf pattern matching rules.
 * Generates bottom-up decision trees, memory addressing mode folding, register class assignments,
 * commutative matching, and folded dead-code elimination.
 */
class CppInstructionSelectorGenerator : public CodeGenerator
{
  public:
    CppInstructionSelectorGenerator(DiagnosticCollector *collector,
                                    SymbolTable *table,
                                    std::filesystem::path outPath,
                                    std::string targetName = "Target");

    bool run() override;

    const std::string &getTargetName() const noexcept { return m_targetName; }
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

    void emitHeader(CppSourceEmitter &emitter) const;
    void emitSource(CppSourceEmitter &emitter) const;

    std::vector<const Symbol *> collectPatternSymbols() const;
    std::vector<const Symbol *> collectAddrModeSymbols() const;

  private:
    std::string m_targetName;
};

extern bool GenerateInstructionSelector(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        std::filesystem::path outPath,
                                        std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_INSTRUCTION_SELECTOR_GENERATOR_H
