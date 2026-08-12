#ifndef EZPACKER_FRAMELOWERERPASSVERIFIER_H
#define EZPACKER_FRAMELOWERERPASSVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class FrameLowererPassVerifier : public MirPassVerifier<MirFrameLowererPass, FrameLowererPassVerifier>
{
  public:
    FrameLowererPassVerifier(MirBuilderContext *ctx, MirFrameLowererPass *pass);

    /**
     * Verifies that the top of the function's entry block contains a valid prologue:
     * 1. Frame Pointer setup (PUSH FP; MOV FP, SP) if enabled by ABI.
     * 2. PUSH instructions for all used callee-saved physical registers.
     * 3. SUB SP, stackAllocSize instruction if local frame payload > 0.
     *
     * @param func Target function to verify.
     * @param layout Expected frame layout metadata computed during pass execution.
     * @return Reference to self for method chaining.
     */
    FrameLowererPassVerifier &verifyPrologue(MirFunction *func, const FrameLayout &layout);

    /**
     * Verifies that every block terminating with a return instruction contains a valid epilogue:
     * 1. ADD SP, stackAllocSize instruction if local frame payload > 0.
     * 2. POP instructions restoring used callee-saved registers in REVERSE order.
     * 3. POP FP instruction if frame pointer was enabled by ABI.
     *
     * @param func Target function to verify.
     * @param layout Expected frame layout metadata computed during pass execution.
     * @return Reference to self for method chaining.
     */
    FrameLowererPassVerifier &verifyEpilogue(MirFunction *func, const FrameLayout &layout);

    /**
     * Verifies that no abstract StackObject MirReference operands remain in any block instruction.
     * All references must be rewritten to concrete MirMemory operands ([baseReg + offset]).
     *
     * @param func Target function to verify.
     * @return Reference to self for method chaining.
     */
    FrameLowererPassVerifier &verifyStackReferencesLowered(MirFunction *func, const FrameLayout &layout);

    /**
     * Asserts that no DALLOC instructions remain, size alignment / SUB SP sequence was emitted,
     * and that the function has enforce-FP flagged.
     */
    FrameLowererPassVerifier &verifyDAllocLowered(MirFunction *func, const FrameLayout &layout);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_FRAMELOWERERPASSVERIFIER_H
