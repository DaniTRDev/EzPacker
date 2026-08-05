#ifndef EZPACKER_MIRFRAMELOWERER_H
#define EZPACKER_MIRFRAMELOWERER_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"

/**
 * Stores calculated stack frame dimensions and concrete object offsets
 * for a single function.
 */
struct FrameLayout
{
    size_t calleeSavedAreaSize = 0; ///< Total size (in bytes) occupied by pushed callee-saved registers.
    size_t totalFrameSize = 0;      ///< Total aligned stack payload size allocated during the prologue.
};

/**
 * Execution context provided to the frame lowerer containing function,
 * target hardware, and memory resource state.
 */
struct FrameLowererCtx
{
    FrameLayout m_layout;
    MirBuilderContext *m_ctx;  ///< Shared compiler context for operand and instruction building.
    MirFunction *m_targetFunc; ///< Function being processed.
    TargetDesc *m_targetDesc;  ///< Hardware target descriptor containing the TargetFrameLowering implementation.

    std::pmr::memory_resource *m_allocator; ///< Memory allocator for temporary layout data structures.

    FrameLowererCtx(MirBuilderContext *ctx,
                    MirFunction *func,
                    TargetDesc *targetDesc,
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
 * its own frame lowerer.
 *
 * Execution flow:
 * 1. Calculate final frame layout and stack object offsets.
 * 2. Emit target-specific function prologue at the entry block.
 * 3. Emit target-specific function epilogue before all return instructions.
 */
class MirFrameLowerer
{
  public:
    /**
     * Runs the frame lowering pipeline on the provided function context.
     */
    void calculateFrameLayout(FrameLowererCtx &ctx);

    /**
     * Inserts the prologue of the function. calculateFrameLayout must have been called before.
     */
    virtual void insertPrologue(FrameLowererCtx &ctx) = 0;

    /**
     * Inserts the epilogue of the function. calculateFrameLayout must have been called before.
     */
    virtual void insertEpilogue(FrameLowererCtx &ctx) = 0;

    /**
     * Lowers each reference to a stack frame object into a MirMemory as FP/SP + offset.
     */
    void lowerStackObjectReferences(FrameLowererCtx &ctx);
};

#endif // EZPACKER_MIRFRAMELOWERER_H
