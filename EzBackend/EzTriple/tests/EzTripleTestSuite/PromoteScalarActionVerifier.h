#ifndef EZPACKER_PROMOTESCALARACTIONVERIFIER_H
#define EZPACKER_PROMOTESCALARACTIONVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class PromoteScalarActionVerifier : public MirPassVerifier<MirLegalizerPass, PromoteScalarActionVerifier>
{
  public:
    /**
     * Creates the verifier with the given builder ctx and pass.
     * @param ctx
     * @param pass
     */
    PromoteScalarActionVerifier(MirBuilderContext *ctx, MirLegalizerPass *pass);

    /**
     * Sets the target block to the one given, any subsequent call to the verify methods will use this block.
     * @param block
     * @return
     */
    PromoteScalarActionVerifier &beginBlock(MirBlock *block);

    /**
     * This verifier checks that the instruction at given index inside the current block matches the type
     * combination and extension OPCODE (SEXT, ZEXT, FPEXT). This verifier checks if a SRC operand was WIDENED.
     *
     * This verifier ONLY WORKS with instructions with the format: dest, src
     * @param index
     * @param origType
     * @param newType
     * @return
     */
    PromoteScalarActionVerifier &
    verifyExtension(size_t index, MirInstructionOpCode opcode, MirType *origType, MirType *newType);
    
  private:
    MirBlock *m_targetBlock;
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_PROMOTESCALARACTIONVERIFIER_H
