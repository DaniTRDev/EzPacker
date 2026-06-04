#ifndef EZPACKER_MIRINSTRUCTIONBUILDER_H
#define EZPACKER_MIRINSTRUCTIONBUILDER_H

#include "EzCoreCommon.h"
#include "Emitter/MirEmitterContext.h"

class MirInstructionBuilder
{
  public:
    /**
     * Creates the builder with the given targetBlock, ctx and opcode.
     * @param targetBlock
     * @param ctx
     * @param opcode
     */
    MirInstructionBuilder(MirEmitterContext *ctx, MirInstructionOpCode opcode);

    /**
     * Pushes the built instruction into the context.
     */
    void flush();

    /**
     * Overload of the '<<' operator that allows pushing operands easily.
     * @param operand
     * @return
     */
    MirInstructionBuilder &operator<<(const MirOperand &operand);

  private:
    MirEmitterContext *m_ctx;
    MirInstruction *m_instr;
};

#endif // EZPACKER_MIRINSTRUCTIONBUILDER_H
