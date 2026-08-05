#ifndef EZPACKER_LIVENESSPASSVERIFIER_H
#define EZPACKER_LIVENESSPASSVERIFIER_H

#include "MirCoreVerifiers.h"

class LivenessAnalysisVerifier : public MirPassVerifier<LivenessAnalysisPass, LivenessAnalysisVerifier>
{
  public:
    /**
     * Creates the verifier and links it to the given liveness pass.
     * @param pass
     */
    LivenessAnalysisVerifier(LivenessAnalysisPass *pass);

    /**
     * Verifies that a specific register is present in a block's Local DEF set.
     * @param blockId
     * @param regId
     */
    LivenessAnalysisVerifier &localDef(size_t blockId, RegisterRefClass refClass, size_t regId);

    /**
     * Verifies that a specific register is NOT present in a block's Local DEF set.
     * @param blockId
     * @param regId
     */
    LivenessAnalysisVerifier &notLocalDef(size_t blockId, RegisterRefClass refClass, size_t regId);

    /**
     * Verifies that a specific register is present in a block's Local USE set.
     * @param blockId
     * @param regId
     */
    LivenessAnalysisVerifier &localUse(size_t blockId, RegisterRefClass refClass, size_t regId);

    /**
     * Verifies that a specific register is NOT present in a block's Local USE set.
     * @param blockId
     * @param regId
     */
    LivenessAnalysisVerifier &notLocalUse(size_t blockId, RegisterRefClass refClass, size_t regId);

    /**
     * Verifies that a register is alive when entering a specific basic block.
     * @param blockId
     * @param regId
     */
    LivenessAnalysisVerifier &liveIn(size_t blockId, RegisterRefClass refClass, size_t regId);

    /**
     * Verifies that a register is NOT alive when entering a specific basic block.
     * @param blockId
     * @param regId
     */
    LivenessAnalysisVerifier &notLiveIn(size_t blockId, RegisterRefClass refClass, size_t regId);

    /**
     * Verifies that a register is alive when exiting a specific basic block.
     * @param blockId
     * @param regId
     */
    LivenessAnalysisVerifier &liveOut(size_t blockId, RegisterRefClass refClass, size_t regId);

    /**
     * Verifies that a register is NOT alive when exiting a specific basic block.
     * @param blockId
     * @param regId
     */
    LivenessAnalysisVerifier &notLiveOut(size_t blockId, RegisterRefClass refClass, size_t regId);

    /**
     * Asserts the exact count of items in the Live-In set of a block.
     * @param blockId
     * @param expectedCount
     */
    LivenessAnalysisVerifier &liveInCount(size_t blockId, size_t expectedCount);

    /**
     * Asserts the exact count of items in the Live-Out set of a block.
     * @param blockId
     * @param expectedCount
     */
    LivenessAnalysisVerifier &liveOutCount(size_t blockId, size_t expectedCount);

  private:
};

#endif // EZPACKER_LIVENESSPASSVERIFIER_H
