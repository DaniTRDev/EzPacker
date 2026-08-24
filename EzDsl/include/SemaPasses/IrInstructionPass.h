#ifndef EZDSL_IR_INSTRUCTION_PASS_h
#define EZDSL_IR_INSTRUCTION_PASS_h

#include "EzDslCommon.h"
#include "Ast/IrInstructionDefLangAst.h"

/**
 * This pass resolves the IR (Mir) instructions parsed and defines a symbol for each one so they can be referenced in
 * other parts of the DSL. It also performs checks to ensure the integrity of the DSL.
 */
class IrInstructionPass
{
  public:
    /**
     * Runs the pass and returns true if succeeded.
     */
    bool run(class DiagnosticCollector *collector, class SymbolTable *table, DSL::Ast::IrInstDef::IrInstDefFile *file);

  private:
    bool validateInstruction(class DiagnosticCollector *collector,
                             class SymbolTable *table,
                             const DSL::Ast::IrInstDef::IrInstDecl &inst);

    bool validateOperands(class DiagnosticCollector *collector,
                          const DSL::Ast::IrInstDef::IrInstDecl &inst,
                          size_t &numIn,
                          size_t &numOut);

    bool validateFlagsAndCategory(class DiagnosticCollector *collector,
                                  const DSL::Ast::IrInstDef::IrInstDecl &inst,
                                  DSL::Ast::IrInstDef::IrInstFlag combinedFlags,
                                  size_t numIn,
                                  size_t numOut);
};
#endif // EZDSL_IR_INSTRUCTION_PASS_h