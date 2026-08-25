#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

/**
 * Test fixture for MIR Basic Block construction, instruction insertion, and metadata management.
 */
class BlockTest : public MirTestSuiteAsGtest
{
  public:
};

namespace
{
/**
 * Custom GoogleTest assertion verifying that a MIR basic block contains the expected number of instructions.
 */
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

/**
 * Verifies that a basic block can be constructed within a function and starts with 0 instructions.
 */
TEST_F(BlockTest, AddBlock)
{
    MirBlockBuilder builder(getBuilderCtx(), getTestFunc());
    MirBlock *block = builder.build(nullptr, "BasicBlock");

    EXPECT_TRUE(HasInstructionCount(block, 0));
}

/**
 * Verifies that inserting an instruction into the entry block before building a new block
 * results in the instruction residing solely in the entry block.
 */
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

/**
 * Verifies that inserting an instruction using a newly created block's builder
 * targets the new block without modifying the entry point.
 */
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

/**
 * Verifies independent instruction insertions across multiple basic blocks within the same function.
 */
TEST_F(BlockTest, AddBlockAndInstructionOnBothBlocks)
{
    // Order:
    // 1. instrBuilder pushes into ENTRYPOINT.
    // 2. blockBuilder creates BLOCK 2.
    // 3. We use blockBuilder's instrBuilder to push into BLOCK 2.

    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());

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

/**
 * Verifies sequential insertion of multiple instructions (ADD, SUB, MUL) into a single block
 * and verifies iterator stability.
 */
TEST_F(BlockTest, TestMultipleInstructionsInSingleBlock)
{
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

/**
 * Verifies that basic blocks receive unique sequential IDs and correctly preserve given string names.
 */
TEST_F(BlockTest, TestBlockNamingAndID)
{
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