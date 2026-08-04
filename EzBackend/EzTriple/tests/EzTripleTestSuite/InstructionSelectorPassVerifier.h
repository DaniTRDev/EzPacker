#ifndef EZPACKER_INSTRUCTIONSELECTORPASSVERIFIER_H
#define EZPACKER_INSTRUCTIONSELECTORPASSVERIFIER_H

#include "EzMirTestSuite.h"
#include "EzTriple.h"

class InstructionSelectorPassVerifier
    : public MirPassVerifier<MirInstructionSelectorPass, InstructionSelectorPassVerifier>
{
  public:
    InstructionSelectorPassVerifier(MirBuilderContext *ctx, MirInstructionSelectorPass *pass);

    /**
     * Verifies that a specific instruction at instructionIndex in targetBlock was selected to the
     * expected MirTargetInstructionId.
     *
     * @param targetBlock The block containing the instruction.
     * @param instructionIndex Zero-based index of the instruction in the block.
     * @param expectedTargetId The expected target instruction ID assigned by selection.
     * @return Reference to self for method chaining.
     */
    InstructionSelectorPassVerifier &
    verifyInstructionSelected(MirBlock *targetBlock, size_t instructionIndex, MirTargetInstructionId expectedTargetId);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_INSTRUCTIONSELECTORPASSVERIFIER_H