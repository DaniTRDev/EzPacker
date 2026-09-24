#include "EzTripleTestSuite.h"
#include "Block/MirBlock.h"
#include "FrameLowerer/MirFrameLowerer.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

/**
 * Fixture for stack frame layout, stack-reference lowering, and the frame lowerer pass.
 */
class MirFrameLowererTest : public EzTripleTestSuite
{
  protected:
    // Resets the mock frame lowerer counters before each test.
    void SetUp() override
    {
        EzTripleTestSuite::SetUp();
        auto *mockLowerer = getTargetDesc()->getMockFrameLowerer();
        if (mockLowerer)
        {
            mockLowerer->reset();
        }
    }
};

// ============================================================================
// 1. Stack Frame Layout Calculation Tests
// ============================================================================

TEST_F(MirFrameLowererTest, TestBasicFrameLayoutCalculation)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("frame_layout_basic", typeTable->i32());

    // Allocate 3 stack objects of varying sizes and alignments
    auto *obj1 = func->getStackFrame()->createStaticStackObj(typeTable->i32()); // 4 bytes
    auto *obj2 = func->getStackFrame()->createStaticStackObj(typeTable->i64()); // 8 bytes
    auto *obj3 = func->getStackFrame()->createStaticStackObj(typeTable->i8());  // 1 byte

    ASSERT_NE(obj1, nullptr);
    ASSERT_NE(obj2, nullptr);
    ASSERT_NE(obj3, nullptr);

    FrameLowererCtx flCtx(ctx, func, getTargetDesc());
    auto *lowerer = getTargetDesc()->getFrameLowerer();

    lowerer->calculateFrameLayout(flCtx);

    auto *analysisData = func->getAnalysisData();
    ASSERT_NE(analysisData, nullptr);

    // On downward-growing stacks, offsets relative to FP are negative
    EXPECT_LT(obj1->m_offset, 0);
    EXPECT_LT(obj2->m_offset, 0);
    EXPECT_LT(obj3->m_offset, 0);

    // Verify object alignments:
    // Slot size is 8 on MockTarget, so alignments are max(typeAlign, slotSize) = 8
    EXPECT_EQ(std::abs(obj1->m_offset) % 8, 0);
    EXPECT_EQ(std::abs(obj2->m_offset) % 8, 0);
    EXPECT_EQ(std::abs(obj3->m_offset) % 8, 0);

    // Verify offsets do not overlap
    EXPECT_NE(obj1->m_offset, obj2->m_offset);
    EXPECT_NE(obj2->m_offset, obj3->m_offset);
    EXPECT_NE(obj1->m_offset, obj3->m_offset);

    // Total frame size must be aligned to calling convention stack alignment (16 bytes)
    EXPECT_GT(analysisData->m_totalFrameSize, 0);
    EXPECT_EQ(analysisData->m_totalFrameSize % getTargetDesc()->getMockCallingConv()->getStackAlignment(), 0);
}

// Verifies used callee-saved registers are accounted for and stack objects sit past the saved area.
TEST_F(MirFrameLowererTest, TestFrameLayoutWithCalleeSavedRegisters)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("frame_layout_callee_saves", typeTable->i32());

    // Register two used callee-saved registers (e.g. RBP, RSP)
    auto *cc = getTargetDesc()->getMockCallingConv();
    MirFunctionBuilder(ctx).addPhysRegUse(func, cc->getFramePointerReg()).addPhysRegUse(func, cc->getStackPointerReg());

    auto *obj = func->getStackFrame()->createStaticStackObj(typeTable->i64());

    FrameLowererCtx flCtx(ctx, func, getTargetDesc());
    auto *lowerer = getTargetDesc()->getFrameLowerer();

    lowerer->calculateFrameLayout(flCtx);

    auto *analysisData = func->getAnalysisData();
    const size_t slotSize = getTargetDesc()->getStackSlotSize();

    // 2 callee-saved registers * 8 bytes = 16 bytes callee saved area
    EXPECT_EQ(analysisData->m_calleeSavedAreaSize, 2 * slotSize);

    // Stack objects start after the callee-saved area (offset <= -24)
    EXPECT_LE(obj->m_offset, -static_cast<int64_t>(analysisData->m_calleeSavedAreaSize + 8));
}

// ============================================================================
// 2. Stack Object Reference Lowering Tests
// ============================================================================

TEST_F(MirFrameLowererTest, TestLowerStackObjectReferences)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("stack_ref_lower", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // Create a local stack variable
    auto *stackObj = func->getStackFrame()->createStaticStackObj(typeTable->i32());
    MirReference *stackRef = ob.buildRef(stackObj);
    MirRegister *vreg = ob.buildVReg(typeTable->i32(), "dest");

    // MOV %dest, %stackObjRef
    ib.MOV(vreg, stackRef);

    ASSERT_EQ(block->getInstructions().size(), 1);
    MirInstruction *inst = *block->getInstructions().begin();
    EXPECT_TRUE(inst->getOperand(1)->isOfType<MirReference>());

    FrameLowererCtx flCtx(ctx, func, getTargetDesc());
    auto *lowerer = getTargetDesc()->getFrameLowerer();

    // Step 1: Compute layout to calculate concrete offsets
    lowerer->calculateFrameLayout(flCtx);

    // Step 2: Lower stack references into concrete memory operands
    lowerer->lowerStackObjectReferences(flCtx);

    // Verify the operand was swapped to MirMemory
    MirOperand *loweredOp = inst->getOperand(1);
    ASSERT_NE(loweredOp, nullptr);
    EXPECT_TRUE(loweredOp->isOfType<MirMemory>());

    MirMemory *memOp = loweredOp->get<MirMemory>();
    ASSERT_NE(memOp, nullptr);

    // Base register should be the frame pointer designated by the calling convention
    MirRegister *baseReg = memOp->getBase();
    ASSERT_NE(baseReg, nullptr);
    EXPECT_TRUE(baseReg->isPhysical());
    EXPECT_EQ(baseReg->getRegId(), getTargetDesc()->getMockCallingConv()->getFramePointerReg().getId());

    // Displacement should equal the object's assigned frame offset
    MirOperand *dispOp = memOp->getDisplacement();
    ASSERT_NE(dispOp, nullptr);
    EXPECT_TRUE(dispOp->isOfType<MirInteger>());
    EXPECT_EQ(dispOp->get<MirInteger>()->getValue().getI64(), stackObj->m_offset);
}

// ============================================================================
// 3. Frame Lowerer Pass Pipeline & ALLOC / DALLOC Dispatch Tests
// ============================================================================

TEST_F(MirFrameLowererTest, TestFrameLowererPassRun)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("frame_pass_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *stackObj = func->getStackFrame()->createStaticStackObj(typeTable->i64());
    MirReference *stackRef = ob.buildRef(stackObj);
    MirRegister *vreg = ob.buildVReg(typeTable->i64(), "dest");
    ib.MOV(vreg, stackRef);

    // Run MirFrameLowererPass
    MirFrameLowererPass pass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
    EXPECT_TRUE(result.m_executed);
    EXPECT_TRUE(result.m_modifiedMir);

    // Verify mock prologue & epilogue were invoked
    auto *mockLowerer = getTargetDesc()->getMockFrameLowerer();
    EXPECT_TRUE(mockLowerer->m_prologueInserted);
    EXPECT_TRUE(mockLowerer->m_epilogueInserted);

    // Verify stack reference was converted to MirMemory
    MirInstruction *loweredInst = *block->getInstructions().begin();
    EXPECT_TRUE(loweredInst->getOperand(1)->isOfType<MirMemory>());
}

// Verifies ALLOC and DALLOC instructions dispatch to the mock lowerAlloc/lowerDAlloc hooks.
TEST_F(MirFrameLowererTest, TestAllocAndDAllocDispatch)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("alloc_dispatch_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *r1 = ob.buildVReg(typeTable->getPtr(typeTable->i8()), "buf1");
    MirRegister *r2 = ob.buildVReg(typeTable->getPtr(typeTable->i8()), "buf2");
    MirInteger *sz = ob.buildInt(typeTable->i64(), FlexInt(64));

    // Emit ALLOC and DALLOC instructions
    ib.ALLOC(r1, sz);
    ib.DALLOC(r2, sz);

    EXPECT_EQ(block->getInstructions().size(), 2);

    auto *mockLowerer = getTargetDesc()->getMockFrameLowerer();
    mockLowerer->reset();

    MirFrameLowererPass pass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);

    // Verify that lowerAlloc was called for ALLOC and lowerDAlloc was called for DALLOC
    EXPECT_EQ(mockLowerer->m_allocLoweredCount, 1);
    EXPECT_EQ(mockLowerer->m_dallocLoweredCount, 1);
}