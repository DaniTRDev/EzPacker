#ifndef EZDSL_CPP_MIR_IR_INSTRUCTION_GENERATOR_H
#define EZDSL_CPP_MIR_IR_INSTRUCTION_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes the EzMir IR instruction definition file (MirInstructionSetDefs.h)
 * from the parsed IR instruction symbols in the SymbolTable.
 * Emits opcode definitions, tier markers, flag masks, and instruction descriptor table registrations.
 */
class CppMirInstructionGenerator : public CodeGenerator
{
  public:
    CppMirInstructionGenerator(DiagnosticCollector *collector,
                               SymbolTable *table,
                               std::filesystem::path outPath);

    bool run() override;

    void emitInstructionDefs(CppSourceEmitter &emitter, const std::vector<const Symbol *> &instSymbols) const;

    std::vector<const Symbol *> collectInstructionSymbols() const;
};

extern bool GenerateMirIrInstructionDefs(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         std::filesystem::path outPath);

} // namespace CodeGenerators

#endif // EZDSL_CPP_MIR_IR_INSTRUCTION_GENERATOR_H