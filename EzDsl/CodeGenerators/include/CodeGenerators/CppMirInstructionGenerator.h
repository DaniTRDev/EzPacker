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
    /**
     * Constructs a generator that writes the EzMir IR instruction definition header.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed IR instruction symbols.
     * @param outPath Destination file or directory for the generated header.
     */
    CppMirInstructionGenerator(DiagnosticCollector *collector, SymbolTable *table, std::filesystem::path outPath);

    /** Generates MirInstructionSetDefs.h; returns false if validation or emission fails. */
    bool run() override;

    /** Emits the INSTRUCTION(...) X-macro entries for every collected IR instruction. */
    void emitInstructionDefs(CppSourceEmitter &emitter, const std::vector<const Symbol *> &instSymbols) const;

    /** Collects all SymbolType::IrInstruction symbols from the symbol table in declaration order. */
    std::vector<const Symbol *> collectInstructionSymbols() const;
};

/**
 * Convenience entry point that constructs a CppMirInstructionGenerator and runs it.
 */
extern bool
GenerateMirIrInstructionDefs(DiagnosticCollector *collector, SymbolTable *table, std::filesystem::path outPath);

} // namespace CodeGenerators

#endif // EZDSL_CPP_MIR_IR_INSTRUCTION_GENERATOR_H