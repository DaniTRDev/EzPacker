#ifndef EZTRIPLE_MIR_FRAME_LOWERER_H
#define EZTRIPLE_MIR_FRAME_LOWERER_H

#include "EzTripleCommon.h"

/**
 * Execution context provided to the frame lowerer containing function,
 * target hardware, and memory resource state.
 */
struct FrameLowererCtx
{
    class MirBuilderContext *m_ctx;  // Shared compiler context for operand and instruction building.
    class MirFunction *m_targetFunc; // Function being processed.
    class TargetDesc *m_targetDesc;  // Hardware target descriptor containing the TargetFrameLowering implementation.

    // Iterator pointing to an ALLOC/DALLOC instruction. Used by lowerAlloc/lowerDAlloc.
    std::pmr::list<class MirInstruction *>::iterator m_allocIt;
    std::pmr::memory_resource *m_allocator; ///< Memory allocator for temporary layout data structures.

    FrameLowererCtx(class MirBuilderContext *ctx,
                    class MirFunction *func,
                    class TargetDesc *targetDesc,
                    std::pmr::memory_resource *alloc) :
        m_ctx(ctx), m_targetFunc(func), m_targetDesc(targetDesc), m_allocator(alloc)
    {
    }
};

/**
 * Driver class responsible for executing Prologue/Epilogue Insertion (PEI) and
 * converting abstract stack object references into concrete memory operands.
 *
 * The insertion of the prologue and epilogue is highly dependent on the target, so each target needs to define
 * its own frame lowerer. Since this pass executes AFTER MirInstructionSelectorPass, this pass need to emit VALID TARGET
 * INSTRUCTIONS, if a high level / pass internal instruction is emitted, undefined behaviour is assured when emitting
 * the code.
 *
 * Execution flow:
 * 1. Lower every ALLOC instruction.
 * 2. Lower every DALLOC instruction.
 * 3. Calculate final frame layout and stack object offsets.
 * 4. Emit target-specific function prologue at the entry block.
 * 5. Emit target-specific function epilogue before all return instructions.
 *
 * The methods in this class are all virtual so a tricky target can lower the frame as it likes without using the
 * predefined code at all.
 */
class MirFrameLowerer
{
  public:
    /**
     * Runs the frame lowering pipeline on the provided function context.
     */
    virtual void calculateFrameLayout(FrameLowererCtx &ctx);

    /**
     * Inserts the prologue of the function. calculateFrameLayout must have been called before.
     */
    virtual void insertPrologue(FrameLowererCtx &ctx) = 0;

    /**
     * Inserts the epilogue of the function. calculateFrameLayout must have been called before.
     */
    virtual void insertEpilogue(FrameLowererCtx &ctx) = 0;

    /**
     * Lowers an ALLOC instruction. This must be called BEFORE calculateFrameLayout. Returns true if an ALLOC was
     * lowered.
     */
    virtual bool lowerAlloc(FrameLowererCtx &ctx) = 0;

    /**
     * Lowers a DALLOC instruction by emitting a series of instructions. This will force the function to use a frame
     * pointer, even if it means using a scratch register. This function is also highly dependant on the target, that's
     * why each target needs to define how to properly lower the dynamic alloc.
     *
     * This function returns TRUE if a DALLOC was actually lowered.
     *
     * Emitted instructions (in general cases) in the place of the DALLOC:
     * sub sp, allocSize
     * and sp, alignment
     * mov type %dest, sp.
     *
     * And, in the return blocks of the function:
     * pop Callee Saved Regs
     * mov sp, fp
     * ret
     */
    virtual bool lowerDAlloc(FrameLowererCtx &ctx) = 0;

    /**
     * Lowers each reference to a stack frame object into a MirMemory as FP/SP + offset.
     */
    virtual void lowerStackObjectReferences(FrameLowererCtx &ctx);
};

#endif // EZTRIPLE_MIR_FRAME_LOWERER_H
