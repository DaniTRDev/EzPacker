#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

class BlockTest : public MirTestSuiteAsGtest
{
  public:
};

namespace
{
::testing::AssertionResult HasInstructionCount(MirBlock *block, size_t expectedCount)
{
    if (!block)
        return ::testing::AssertionFailure() << "Block is nullptr";

    size_t actualCount = block->getInstrCount();
    if (actualCount != expectedCount)
    {
        return ::testing::AssertionFailure() << "Block (ID: " << block->getId() << ") expected " << expectedCount
                                             << " instructions, but got " << actualCount;
    }
    return ::testing::AssertionSuccess();
}

} // anonymous namespace

TEST_F(BlockTest, AddBlock)
{
    MirBlockBuilder builder(getBuilderCtx(), getTestFunc());
    MirBlock *block = builder.build(nullptr, "BasicBlock");

    EXPECT_TRUE(HasInstructionCount(block, 0));
}

TEST_F(BlockTest, AddBlockAndInstructionBeforeCreatingBlock)
{
    // Order:
    // 1. instrBuilder (attached to entrypoint) builds -> instr is pushed into ENTRYPOINT.
    // 2. blockBuilder builds -> creates an empty BLOCK 2.

    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirInstructionBuilder instrBuilder(ctx,
                                       getTestFunc()->getEntryPoint(),
                                       InsertionType::InsertAfter,
                                       getTestFunc()->getEntryPoint()->begin());

    // add i8 %reg1, i8 %reg2
    instrBuilder.ADD(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()),
                     MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()));

    MirBlock *block1 = getTestFunc()->getEntryPoint();
    MirBlock *block2 = blockBuilder.build(nullptr);

    EXPECT_TRUE(HasInstructionCount(block1, 1));
    EXPECT_TRUE(HasInstructionCount(block2, 0));
}

TEST_F(BlockTest, AddBlockAndInstructionAfterCreatingBlock)
{
    // Order:
    // 1. blockBuilder-> build -> creates BLOCK 2.
    // 2. blockBuilder.instrBuilder() -> pushes into BLOCK 2.

    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *block1 = getTestFunc()->getEntryPoint();
    MirBlock *block2 = blockBuilder.build();

    // add i8 %reg1, i8 %reg2
    blockBuilder.instrBuilder().ADD(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()),
                                    MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()));

    EXPECT_TRUE(HasInstructionCount(block1, 0));
    EXPECT_TRUE(HasInstructionCount(block2, 1));
}

TEST_F(BlockTest, AddBlockAndInstructionOnBothBlocks)
{
    // Order:
    // 1. instrBuilder pushes into ENTRYPOINT.
    // 2. blockBuilder creates BLOCK 2.
    // 3. We use blockBuilder's instrBuilder to push into BLOCK 2.

    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    // FIX: Removed the erroneous '=' which invoked the comma operator
    MirInstructionBuilder instrBuilder(ctx,
                                       getTestFunc()->getEntryPoint(),
                                       InsertionType::InsertAfter,
                                       getTestFunc()->getEntryPoint()->begin());

    // sub i8 %reg1, i8 %reg2
    instrBuilder.SUB(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()),
                     MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()));

    MirBlock *block1 = getTestFunc()->getEntryPoint();
    MirBlock *block2 = blockBuilder.build();

    // add i8 %reg3, i8 %reg4
    blockBuilder.instrBuilder().ADD(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()),
                                    MirOperandBuilder(ctx).buildVReg(getTypeTable()->i8()));

    EXPECT_TRUE(HasInstructionCount(block1, 1));
    EXPECT_TRUE(HasInstructionCount(block2, 1));
}

// --- New Coverage Tests ---

TEST_F(BlockTest, TestMultipleInstructionsInSingleBlock)
{
    // Verifies that a block can hold multiple sequential instructions
    // and that the count increments properly.
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirBlock *block = blockBuilder.build();

    // Append 3 instructions
    blockBuilder.instrBuilder().ADD(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i32()),
                                    MirOperandBuilder(ctx).buildVReg(getTypeTable()->i32()));

    blockBuilder.instrBuilder().SUB(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i32()),
                                    MirOperandBuilder(ctx).buildVReg(getTypeTable()->i32()));

    blockBuilder.instrBuilder().MUL(MirOperandBuilder(ctx).buildVReg(getTypeTable()->i32()),
                                    MirOperandBuilder(ctx).buildVReg(getTypeTable()->i32()));

    EXPECT_TRUE(HasInstructionCount(block, 3));

    // Verify iterators aren't broken on multiple insertions
    auto it = block->begin();
    EXPECT_NE(it, block->end());
}

TEST_F(BlockTest, TestBlockNamingAndID)
{
    // Verifies that blocks receive unique identifiers and retain given string names.
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirBlock *blockA = blockBuilder.build(nullptr, "LoopHeader");
    MirBlock *blockB = blockBuilder.build(nullptr, "LoopBody");

    // Block IDs must be strictly unique within the context
    EXPECT_NE(entryPoint->getId(), blockA->getId());
    EXPECT_NE(blockA->getId(), blockB->getId());

    // Check optional naming assignment
    EXPECT_EQ(blockA->getName(), "LoopHeader");
    EXPECT_EQ(blockB->getName(), "LoopBody");
}