#ifndef EZTRIPLE_MIR_INSTRUCTION_SELECTOR_H
#define EZTRIPLE_MIR_INSTRUCTION_SELECTOR_H

#include "EzTripleCommon.h"
#include <list>

class MirBlock;
class MirBuilderContext;
class MirFunction;
class MirInstruction;
class TargetDesc;

/**
 * Abstract interface for target instruction selection.
 * Synthesized target selectors implement pattern matching decision trees to replace generic MIR instructions
 * with hardware instructions and register class constraints.
 */
class MirInstructionSelector
{
  public:
    virtual ~MirInstructionSelector() = default;

    /**
     * Selects and replaces a generic instruction with target hardware instructions.
     * @param ctx Active builder context.
     * @param inst Instruction to select and replace.
     * @return True if the instruction was recognized and successfully transformed.
     */
    virtual bool select(MirBuilderContext *ctx, MirInstruction *inst) = 0;

    /**
     * Runs instruction selection across all blocks in a function.
     */
    virtual bool selectFunction(MirBuilderContext *ctx, MirFunction *func);

    /**
     * Runs instruction selection across all instructions in a basic block.
     */
    virtual bool selectBlock(MirBuilderContext *ctx, MirBlock *block);
};

#endif // EZTRIPLE_MIR_INSTRUCTION_SELECTOR_H
