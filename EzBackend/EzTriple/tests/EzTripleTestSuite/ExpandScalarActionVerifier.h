#ifndef EZPACKER_EXPANDSCALARACTIONVERIFIER_H
#define EZPACKER_EXPANDSCALARACTIONVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class ExpandScalarActionVerifier : public MirPassVerifier<MirBlockLegalizerPass, ExpandScalarActionVerifier>
{
  public:
    /**
     * Creates the verifier with the given builder ctx and pass.
     * @param ctx
     * @param pass
     */
    ExpandScalarActionVerifier(MirBuilderContext *ctx, MirBlockLegalizerPass *pass);

    /**
     * Sets the target block to the one given, any subsequent call to the verify methods will use this block.
     * @param block
     * @return
     */
    ExpandScalarActionVerifier &beginBlock(MirBlock *block);

    /**
     * Verifies that the current target block contains EXACTLY the given instructions.
     * @param expectedOpcodes
     * @return
     */
    ExpandScalarActionVerifier &expectInstructionSequence(const std::vector<MirInstructionOpCode> &expectedOpcodes);

    /**
     * Verifies that the instruction at given index within the current target block is a runtime call with the given
     * runtime symbol name.
     * @param index
     * @param expectedSymbolName
     * @return
     */
    ExpandScalarActionVerifier &verifyRuntimeCallSymbol(size_t index, const std::string &expectedSymbolName);

  private:
    MirBlock *m_targetBlock;
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_EXPANDSCALARACTIONVERIFIER_H
