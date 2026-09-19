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
    /**
     * Constructs a generator that synthesizes an instruction selector for the given target.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed .isf pattern definitions.
     * @param outPath Destination file or directory for the generated artifacts.
     * @param targetName Target identifier substituted into generated class names.
     */
    CppInstructionSelectorGenerator(DiagnosticCollector *collector,
                                    SymbolTable *table,
                                    std::filesystem::path outPath,
                                    std::string targetName = "Target");

    /** Generates the selector header/source pair; returns false if validation or emission fails. */
    bool run() override;

    /** Returns the target identifier used to name generated classes. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to name generated classes. */
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

    /** Emits the selector class declaration and pattern table into the header. */
    void emitHeader(CppSourceEmitter &emitter) const;

    /** Emits the selector matching and rewriting logic into the source. */
    void emitSource(CppSourceEmitter &emitter) const;

    /** Collects the instruction-select pattern symbols produced by the Sema pass, in declaration order. */
    std::vector<const Symbol *> collectPatternSymbols() const;

    /** Collects the addressing-mode symbols that patterns may fold into memory operands. */
    std::vector<const Symbol *> collectAddrModeSymbols() const;

  private:
    /** Target identifier substituted into generated class and include names. */
    std::string m_targetName;
};

extern bool GenerateInstructionSelector(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        std::filesystem::path outPath,
                                        std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_INSTRUCTION_SELECTOR_GENERATOR_H
