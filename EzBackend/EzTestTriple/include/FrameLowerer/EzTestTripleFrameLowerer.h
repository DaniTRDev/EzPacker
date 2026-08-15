#ifndef EZPACKER_EZTESTTRIPLEFRAMELOWERER_H
#define EZPACKER_EZTESTTRIPLEFRAMELOWERER_H

#include "EzTestTripleCommon.h"
#include "InstructionSelector/EzTestTripleInstructionSelector.h"

class EzTestTripleFrameLowerer : public MirFrameLowerer
{
    /**
     * Emits the function prologue:
     * 1. PUSH RFP (if frame pointer is required)
     * 2. MOV RFP, RSP
     * 3. PUSH Callee-Saved Registers
     * 4. SUB RSP, StackFrameSize (Locals, Spills, Outgoing Argument Area)
     */
    void insertPrologue(FrameLowererCtx &ctx) override;

    /**
     * Emits the function epilogue before each return instruction:
     * 1. Reclaim local stack allocation (either via LEA/MOV from RFP or ADD RSP, FrameSize)
     * 2. POP Callee-Saved Registers (in reverse order of prologue push)
     * 3. POP RFP (if frame pointer was set up)
     */
    void insertEpilogue(FrameLowererCtx &ctx) override;

    /**
     * Lowers static stack allocations (ALLOC):
     * 1. Creates a static stack object with the type of the alloc.
     * 2. Places a lea dst, stack addr
     * 3. Erases the original ALLOC instruction.
     *
     * Returns true if an ALLOC instruction was lowered.
     */
    bool lowerAlloc(FrameLowererCtx &ctx) override;

    /**
     * Lowers dynamic stack allocations (DALLOC) at runtime:
     * 1. Aligns the requested size to target stack alignment (16 bytes).
     * 2. Adjusts SP (SUB RSP, AlignedSize).
     * 3. Stores the allocated pointer into the destination operand.
     *
     * Returns true if a DALLOC instruction was lowered.
     */
    bool lowerDAlloc(FrameLowererCtx &ctx) override;

  private:
    /// Computes the aligned total stack allocation needed beyond pushed registers
    int64_t calculateStaticFrameAdjustment(const MirFunctionAnalysisData *analysisData, size_t stackAlign) const;
};

#endif // EZPACKER_EZTESTTRIPLEFRAMELOWERER_H