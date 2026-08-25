#ifndef EZDSL_IR_INSTRUCTION_PASS_h
#define EZDSL_IR_INSTRUCTION_PASS_h

#include "EzDslCommon.h"
#include "Ast/IrInstructionDefLangAst.h"

/**
 * Semantic analysis pass validating generic IR instruction declarations (.irdf).
 * Registers IrInstructionSymbol entries in the symbol table and enforces integrity rules:
 * operand direction constraints, terminator/branch invariants, category flags, and duplicate opcode prevention.
 */
class IrInstructionPass
{
  public:
    /**
     * Executes the IR instruction semantic validation pass across all declared opcodes in the AST.
     * Returns true if all instructions were successfully validated and registered.
     */
    bool run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::IrInstDef::IrInstDefFile *file);

  private:
    /**
     * Validates a single IR instruction declaration including operands, categories, tiers, and behavioral flags.
     */
    bool validateInstruction(class DiagnosticCollector *collector,
                             class SymbolTable *table,
                             const DSL::Ast::IrInstDef::IrInstDecl &inst);

    /**
     * Validates operand type masks, distinct naming, and counts input and output operands.
     */
    bool validateOperands(class DiagnosticCollector *collector,
                          const DSL::Ast::IrInstDef::IrInstDecl &inst,
                          size_t &numIn,
                          size_t &numOut);

    /**
     * Enforces structural invariants between instruction categories (e.g. ControlFlow, Memory, Casting)
     * and verification flags (e.g. IsTerminator, ReadsMemory, SizeMatch).
     */
    bool validateFlagsAndCategory(class DiagnosticCollector *collector,
                                  const DSL::Ast::IrInstDef::IrInstDecl &inst,
                                  DSL::Ast::IrInstDef::IrInstFlag combinedFlags,
                                  size_t numIn,
                                  size_t numOut);
};
#endif // EZDSL_IR_INSTRUCTION_PASS_h