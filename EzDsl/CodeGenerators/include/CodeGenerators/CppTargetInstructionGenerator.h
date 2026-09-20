#ifndef EZDSL_CPP_TARGET_INSTRUCTION_GENERATOR_H
#define EZDSL_CPP_TARGET_INSTRUCTION_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes the target instruction descriptor table header and implementation files
 * (<Target>TargetInstructionTable.h and .cpp) from EzDSL .idf semantic definitions.
 * Emits target opcode enums, MirTargetInstructionDesc tables, operand flags and register class bindings.
 */
class CppTargetInstructionGenerator : public CodeGenerator
{
  public:
    /**
     * Constructs a generator that synthesizes the target instruction table for the given target.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed .idf instruction definitions.
     * @param outPath Destination file or directory for the generated artifacts.
     * @param targetName Target identifier substituted into generated class names.
     */
    CppTargetInstructionGenerator(DiagnosticCollector *collector,
                                  SymbolTable *table,
                                  std::filesystem::path outPath,
                                  std::string targetName = "Target");

    /** Generates the target instruction table header/source pair; returns false if validation or emission fails. */
    bool run() override;

    /** Returns the target identifier used to name generated classes. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to name generated classes. */
    void setTargetName(std::string targetName) { m_targetName = SanitizeCppIdentifier(targetName, "Target"); }

    /** Emits the OpCode enum and descriptor lookup/initialization declarations into the header. */
    void emitHeader(CppSourceEmitter &emitter) const;

    /** Emits the descriptor table and register-class resolution logic into the source. */
    void emitSource(CppSourceEmitter &emitter) const;

    /** Collects all SymbolType::TargetInstruction symbols from the symbol table in declaration order. */
    std::vector<const Symbol *> collectInstructionSymbols() const;

  private:
    /** Target identifier substituted into generated class and include names. */
    std::string m_targetName;
};

} // namespace CodeGenerators

#endif // EZDSL_CPP_TARGET_INSTRUCTION_GENERATOR_H
