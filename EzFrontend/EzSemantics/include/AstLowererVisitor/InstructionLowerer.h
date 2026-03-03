#ifndef EZPACKER_INSTRUCTIONLOWERER_H
#define EZPACKER_INSTRUCTIONLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"

class InstructionLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given Instruction node with the given lowering context. It will emit the instruction with
     * its operands.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;

  private:
    /**
     * Lowers a regular instruction and returns true if succeeded.
     * @param instr
     * @param ctx
     * @return bool
     */
    bool lowerRegularInstruction(Instruction *instr, LoweringContext *ctx);

    /**
     * Lowers a call instruction and returns true if succeeded.
     * @param instr
     * @param ctx
     * @return
     */
    bool lowerCallInstruction(CallInstruction *instr, LoweringContext *ctx);
};

#endif // EZPACKER_INSTRUCTIONLOWERER_H
