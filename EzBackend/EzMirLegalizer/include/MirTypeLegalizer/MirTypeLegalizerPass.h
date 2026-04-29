#ifndef EZPACKER_MIRTYPELEGALIZERPASS_H
#define EZPACKER_MIRTYPELEGALIZERPASS_H

#include "EzMirLegalizerCommon.h"
#include "MirLegalizerContext.h"

// Represents a register split into two smaller registers
struct SplitRegister
{
    TypedPoolSlice<MirRegister> *m_split;
};

class MirTypeLegalizerPass : public IMirTransformPass
{
  public:
    /**
     * Constructs a new MirTypeLegalizerPass.
     * @param ctx
     */
    MirTypeLegalizerPass(MirLegalizerContext *ctx);

    /**
     * Executes the type legalization pass on a single function.
     * Returns true if the MIR was modified (requires invalidating analysis passes).
     */
    bool run(MirFunction *func, class MirPassManager *passManager) override;

  private:
    // Creates an immediate operand dynamically
    MirOperand splitImmediate(const MirInteger *oldImm, size_t chunkIndex);

    // Helpers
    MirRegister getOrCreatePromotedRegister(const MirRegister &oldReg);

    // Helpers to get or create split registers
    SplitRegister getOrCreateSplitRegister(const MirRegister &oldReg);

    TypedPoolSlice<MirInstruction>::Iterator expandArithmetic(TypedPoolSlice<MirInstruction>::Iterator instrIt,
                                                              MirBlock *parentBlock);

    TypedPoolSlice<MirInstruction>::Iterator expandLoad(TypedPoolSlice<MirInstruction>::Iterator instrIt,
                                                        MirBlock *parentBlock);

    TypedPoolSlice<MirInstruction>::Iterator expandMov(TypedPoolSlice<MirInstruction>::Iterator instrIt,
                                                       MirBlock *parentBlock);

    TypedPoolSlice<MirInstruction>::Iterator expandRet(TypedPoolSlice<MirInstruction>::Iterator instrIt,
                                                       MirBlock *parentBlock);

    TypedPoolSlice<MirInstruction>::Iterator expandStore(TypedPoolSlice<MirInstruction>::Iterator instrIt,
                                                         MirBlock *parentBlock);

    // Promotion Handler
    TypedPoolSlice<MirInstruction>::Iterator promoteInstruction(TypedPoolSlice<MirInstruction>::Iterator instrIt,
                                                                MirBlock *parentBlock);

    TypedPoolSlice<MirInstruction>::Iterator runOnInstruction(TypedPoolSlice<MirInstruction>::Iterator instrIt,
                                                              MirBlock *parentBlock);

    std::unordered_map<size_t, MirRegister> m_promoteMap;

  private:
    MirLegalizerContext *m_ctx;

    // Function-scoped state: Maps an oversized virtual register ID to its Low/High halves.
    std::unordered_map<size_t, SplitRegister> m_splitMap;
};

#endif // EZPACKER_MIRTYPELEGALIZERPASS_H