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
    CppTargetInstructionGenerator(DiagnosticCollector *collector,
                                  SymbolTable *table,
                                  std::filesystem::path outPath,
                                  std::string targetName = "Target");

    bool run() override;

    const std::string &getTargetName() const noexcept { return m_targetName; }
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

    void emitHeader(CppSourceEmitter &emitter) const;
    void emitSource(CppSourceEmitter &emitter) const;

    std::vector<const Symbol *> collectInstructionSymbols() const;

  private:
    std::string m_targetName;
};

/**
 * Convenience entry point for generating the target instruction table.
 */
extern bool GenerateTargetInstructionTable(DiagnosticCollector *collector,
                                           SymbolTable *table,
                                           std::filesystem::path outPath,
                                           std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_TARGET_INSTRUCTION_GENERATOR_H
