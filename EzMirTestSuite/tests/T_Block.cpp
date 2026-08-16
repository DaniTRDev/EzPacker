#include <gtest/gtest.h>
#include "../include/EzMirTestSuite.h"

class BlockTest : public MirTestSuiteAsGtest
{
  public:
};

TEST_F(BlockTest, AddBlock)
{
    MirBlockBuilder builder(getBuilderCtx(), getTestFunc());
    MirBlock *block = builder.build(); // Block 1.

    MirBlockVerifier verifier(block);
    verifier.instrCount(0);
}

TEST_F(BlockTest, AddBlockAndInstructionBeforeCreatingBlock)
{
    /**
     * In this case this is the order:
     * instrBuilder (attached to entrypoint) builds -> instr is pushed into ENTRYPOINT.
     */

    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirInstructionBuilder instrBuilder = blockBuilder.instrBuilder();

    // add i8 %reg1, i8 %reg2
    instrBuilder.ADD(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()),
                     MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()));

    MirBlock *block1 = getTestFunc()->getEntryPoint(), *block2 = blockBuilder.build(nullptr);
    MirBlockVerifier verifier1(block1), verifier2(block2);

    verifier1.instrCount(1);
    verifier1.instrCountOfType(1, MirInstructionOpCode::ADD);

    verifier2.instrCount(0);
}

TEST_F(BlockTest, AddBlockAndInstructionAfterCreatingBlock)
{
    /**
     * In this case this is the order:
     * blockBuilder-> build -> instrBuilder (attached to block2) -> build -> instr is pushed into BLOCK 2.
     */

    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *block1 = getTestFunc()->getEntryPoint(), *block2 = blockBuilder.build();
    MirBlockVerifier verifier1(block1), verifier2(block2);

    // add i8 %reg1, i8 %reg2
    blockBuilder.instrBuilder().ADD(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()),
                                    MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()));

    verifier1.instrCount(0);

    verifier2.instrCountOfType(1, MirInstructionOpCode::ADD);
    verifier2.instrCount(1);
}

TEST_F(BlockTest, AddBlockAndInstructionOnBothBloks)
{
    /**
     * Gets test function -> instrBuilder -> build (instr is pushed into ENTRYPOINT).
     * blockBuilder -> build -> instrBuilder -> build (instr is pushed into BLOCK 2);
     */

    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirInstructionBuilder instrBuilder = blockBuilder.instrBuilder();

    // sub i8 %reg1, i8 %reg2
    instrBuilder.SUB(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()),
                     MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()));

    MirBlock *block1 = getTestFunc()->getEntryPoint(), *block2 = blockBuilder.build();

    // add i8 %reg3, i8 %reg4
    instrBuilder = blockBuilder.instrBuilder();
    instrBuilder.ADD(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()),
                     MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()));

    MirBlockVerifier verifier1(block1), verifier2(block2);

    verifier1.instrCount(1);
    verifier1.instrCountOfType(1, MirInstructionOpCode::SUB);

    verifier2.instrCount(1);
    verifier2.instrCountOfType(1, MirInstructionOpCode::ADD);
}
