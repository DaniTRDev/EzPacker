/**
 * @file VariableLowerer.h
 * @brief Lowerer for variable references — maps symbols to MIR virtual
 *        registers or global data entries.
 *
 * For local variables and parameters the lowerer looks up the symbol's MIR
 * ID and pushes a MirRegister operand.  For global variables it emits the
 * initializer data through MirGlobalDataEmitter and pushes a MirReference.
 * Type-cast annotations are also handled here (TRUNC, ZEXT, SEXT, …).
 */
#ifndef EZPACKER_VARIABLELOWERER_H
#define EZPACKER_VARIABLELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericAstLowerer.h"
#include "ImmediateLowerer.h"

class VariableLowerer : public GenericAstLowerer
{
  public:
    /**
     * Tries to lower the given Variable node with the given semantic context, emitter and global data emitter.
     * If it's a global variable, it will emit the global data. If it's a local variable, it will emit the register ID.
     *
     * Returns true if succeeded.
     * @param semanticCtx
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, AstLoweringContext *ctx) override;

  private:
    /**
     * Lowers the global variable and emit its data using MirGlobalDataEmitter. Returns true if succeeded.
     * @param var
     * @param ctx
     * @return bool
     */
    bool lowerGlobalVariable(Variable *var, AstLoweringContext *ctx);

    /**
     * Lowers the given local variable and links its symbol to a register ID if this node is the defining node.
     * @param var
     * @param ctx
     * @return bool
     */
    bool lowerLocalVariable(Variable *var, AstLoweringContext *ctx);

    /**
     * Lowers a cast (implicit or explicit) of a variable.
     * @param var
     * @param ctx
     * @return bool
     */
    bool lowerVariableCast(Variable *var, AstLoweringContext *ctx);

  private:
};

#endif // EZPACKER_VARIABLELOWERER_H
