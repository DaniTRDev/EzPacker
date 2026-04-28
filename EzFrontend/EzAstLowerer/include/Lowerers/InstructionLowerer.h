/**
 * @file InstructionLowerer.h
 * @brief Lowerer for assembly-style instructions (mov, add, call, …).
 *
 * Maps the instruction's mnemonic to a MirInstructionOpCode, lowers every
 * operand (variables, immediates, memory references) by delegating to the
 * visitor, pops the resulting MirOperands, and emits the final MIR
 * instruction.  Call instructions are handled separately to account for
 * their distinct grammar (callee name + return type).
 */
#ifndef EZPACKER_INSTRUCTIONLOWERER_H
#define EZPACKER_INSTRUCTIONLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericAstLowerer.h"

class InstructionLowerer : public GenericAstLowerer
{
  public:
    /**
     * Tries to lower the given Instruction node with the given lowering context. It will emit the instruction with
     * its operands.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, AstLoweringContext *ctx) override;

  private:
    /**
     * Lowers a regular instruction and returns true if succeeded.
     * @param instr
     * @param ctx
     * @return bool
     */
    bool lowerRegularInstruction(Instruction *instr, AstLoweringContext *ctx);

    /**
     * Lowers a call instruction and returns true if succeeded.
     * @param instr
     * @param ctx
     * @return
     */
    bool lowerCallInstruction(CallInstruction *instr, AstLoweringContext *ctx);
};

#endif // EZPACKER_INSTRUCTIONLOWERER_H
