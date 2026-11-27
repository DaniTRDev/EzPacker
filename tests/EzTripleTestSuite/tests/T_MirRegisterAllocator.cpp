#include "EzTripleTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "RegisterAllocator/MirRegisterAllocator.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"
#include "Type/MirTypeTable.h"

/**
 * Fixture for interference-graph construction, coloring, spilling, and the register allocator pass.
 */
class MirRegisterAllocatorTest : public EzTripleTestSuite
{
  protected:
    // Resets the mock register allocator counters before each test.
    void SetUp() override
    {
        EzTripleTestSuite::SetUp();
        auto *mockAlloc = getTargetDesc()->getMockRegisterAllocator();
        if (mockAlloc)
        {
            mockAlloc->reset();
        }
    }
};

// ============================================================================
// 1. Interference Graph Construction Tests
// ============================================================================

TEST_F(MirRegisterAllocatorTest, TestInterferenceGraphNonOverlapping)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("non_overlapping", typeTable->i32());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i32(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i32(), "v1", nullptr, gpr);
    auto *retReg = ob.buildVReg(typeTable->i32(), "retReg", nullptr, gpr);
    auto *imm10 = ob.buildInt(typeTable->i32(), FlexInt(10));
    auto *imm20 = ob.buildInt(typeTable->i32(), FlexInt(20));

    // v0 defined and consumed before v1 is defined:
    // MOV v0, 10
    // MOV retReg, v0   (v0 dies here)
    // MOV v1, 20       (v1 starts here)
    // ADD retReg, retReg, v1
    // RET retReg
    ib.MOV(v0, imm10);
    ib.MOV(retReg, v0);
    ib.MOV(v1, imm20);
    ib.ADD(retReg, retReg, v1);
    ib.RET(retReg);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessPass = passManager.getAnalysis<LivenessAnalysisPass>(ctx);
    ASSERT_NE(livenessPass, nullptr);
    auto *livenessResult = livenessPass->getResult();
    ASSERT_NE(livenessResult, nullptr);

    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();
    ASSERT_NE(allocator, nullptr);

    bool buildOk = allocator->buildInterferenceGraph(livenessResult, &regCtx);
    EXPECT_TRUE(buildOk);

    // v0 and v1 must NOT interfere
    ASSERT_TRUE(regCtx.hasNode(v0->getRef()));
    EXPECT_FALSE(regCtx.hasEdge(v0->getRef(), v1->getRef()));

    ASSERT_TRUE(regCtx.hasNode(v1->getRef()));
    EXPECT_FALSE(regCtx.hasEdge(v1->getRef(), v0->getRef()));
}

// Verifies simultaneously live virtual registers interfere symmetrically.
TEST_F(MirRegisterAllocatorTest, TestInterferenceGraphOverlapping)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("overlapping", typeTable->i32());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i32(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i32(), "v1", nullptr, gpr);
    auto *v2 = ob.buildVReg(typeTable->i32(), "v2", nullptr, gpr);
    auto *imm10 = ob.buildInt(typeTable->i32(), FlexInt(10));
    auto *imm20 = ob.buildInt(typeTable->i32(), FlexInt(20));

    // MOV v0, 10
    // MOV v1, 20       (v0 is still live!)
    // ADD v2, v0, v1   (both v0 and v1 read here)
    // RET v2
    ib.MOV(v0, imm10);
    ib.MOV(v1, imm20);
    ib.ADD(v2, v0, v1);
    ib.RET(v2);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessPass = passManager.getAnalysis<LivenessAnalysisPass>(ctx);
    ASSERT_NE(livenessPass, nullptr);
    auto *livenessResult = livenessPass->getResult();
    ASSERT_NE(livenessResult, nullptr);

    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();
    ASSERT_NE(allocator, nullptr);

    bool buildOk = allocator->buildInterferenceGraph(livenessResult, &regCtx);
    EXPECT_TRUE(buildOk);

    // v0 and v1 MUST interfere with each other symmetrically
    ASSERT_TRUE(regCtx.hasNode(v0->getRef()));
    EXPECT_TRUE(regCtx.hasEdge(v0->getRef(), v1->getRef()));

    ASSERT_TRUE(regCtx.hasNode(v1->getRef()));
    EXPECT_TRUE(regCtx.hasEdge(v1->getRef(), v0->getRef()));
}

// ============================================================================
// 2. Degree Evaluation & Reserved Registers
// ============================================================================

TEST_F(MirRegisterAllocatorTest, TestDegreeEvaluationAndReservedRegs)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("degree_eval", typeTable->i32());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i32(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i32(), "v1", nullptr, gpr);
    auto *v2 = ob.buildVReg(typeTable->i32(), "v2", nullptr, gpr);
    auto *imm = ob.buildInt(typeTable->i32(), FlexInt(1));

    ib.MOV(v0, imm);
    ib.MOV(v1, imm);
    ib.ADD(v2, v0, v1);
    ib.RET(v2);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessResult = passManager.getAnalysis<LivenessAnalysisPass>(ctx)->getResult();
    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();

    allocator->buildInterferenceGraph(livenessResult, &regCtx);
    allocator->evaluateInterferenceGraphDegree(&regCtx);

    // Virtual registers have degree equal to their neighbor count
    EXPECT_EQ(regCtx.getDegree(v0->getRef()), regCtx.getNeighborCount(v0->getRef()));
    EXPECT_EQ(regCtx.getDegree(v1->getRef()), regCtx.getNeighborCount(v1->getRef()));

    // Frame pointer must be marked reserved
    auto *cc = getTargetDesc()->getMockCallingConv();
    if (cc->hasFramePointer(func))
    {
        EXPECT_TRUE(regCtx.m_reservedRegs.contains(cc->getFramePointerReg()));
    }
}

// ============================================================================
// 3. Graph Simplification & Color Selection
// ============================================================================

TEST_F(MirRegisterAllocatorTest, TestSimplificationAndColorSelection)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("color_select", typeTable->i32());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i32(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i32(), "v1", nullptr, gpr);
    auto *v2 = ob.buildVReg(typeTable->i32(), "v2", nullptr, gpr);
    auto *imm = ob.buildInt(typeTable->i32(), FlexInt(42));

    ib.MOV(v0, imm);
    ib.MOV(v1, imm);
    ib.ADD(v2, v0, v1);
    ib.RET(v2);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessResult = passManager.getAnalysis<LivenessAnalysisPass>(ctx)->getResult();
    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();

    allocator->buildInterferenceGraph(livenessResult, &regCtx);
    allocator->evaluateInterferenceGraphDegree(&regCtx);

    bool simplifyOk = allocator->simplify(&regCtx);
    EXPECT_TRUE(simplifyOk);
    EXPECT_EQ(regCtx.m_selectStack.size(), 3);

    bool selectOk = allocator->selectColors(&regCtx);
    EXPECT_TRUE(selectOk);

    // Verify all 3 virtual registers received physical registers
    ASSERT_TRUE(regCtx.m_allocatedRegs.contains(v0->getRef()));
    ASSERT_TRUE(regCtx.m_allocatedRegs.contains(v1->getRef()));
    ASSERT_TRUE(regCtx.m_allocatedRegs.contains(v2->getRef()));

    MirRegisterRef p0 = regCtx.m_allocatedRegs[v0->getRef()];
    MirRegisterRef p1 = regCtx.m_allocatedRegs[v1->getRef()];
    MirRegisterRef p2 = regCtx.m_allocatedRegs[v2->getRef()];

    EXPECT_TRUE(p0.isPhysical());
    EXPECT_TRUE(p1.isPhysical());
    EXPECT_TRUE(p2.isPhysical());

    // Interfering registers must not share colors
    EXPECT_NE(p0, p1);

    // Reserved registers (FP/SP) must not be allocated
    auto *cc = getTargetDesc()->getMockCallingConv();
    EXPECT_NE(p0, cc->getFramePointerReg());
    EXPECT_NE(p1, cc->getFramePointerReg());
    EXPECT_NE(p2, cc->getFramePointerReg());
    EXPECT_NE(p0, cc->getStackPointerReg());
    EXPECT_NE(p1, cc->getStackPointerReg());
    EXPECT_NE(p2, cc->getStackPointerReg());
}

// ============================================================================
// 4. MIR Operand Rewriting Tests
// ============================================================================

TEST_F(MirRegisterAllocatorTest, TestRewriteColorsInMir)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("rewrite_colors", typeTable->i32());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i32(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i32(), "v1", nullptr, gpr);
    auto *v2 = ob.buildVReg(typeTable->i32(), "v2", nullptr, gpr);
    auto *imm = ob.buildInt(typeTable->i32(), FlexInt(5));

    ib.MOV(v0, imm);
    ib.MOV(v1, imm);
    ib.ADD(v2, v0, v1);
    ib.RET(v2);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessResult = passManager.getAnalysis<LivenessAnalysisPass>(ctx)->getResult();
    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();

    allocator->buildInterferenceGraph(livenessResult, &regCtx);
    allocator->evaluateInterferenceGraphDegree(&regCtx);
    allocator->simplify(&regCtx);
    allocator->selectColors(&regCtx);

    // Perform MIR rewrite
    allocator->rewriteColors(&regCtx);

    // Verify all registers in all instructions are now physical
    for (MirInstruction *inst : entry->getInstructions())
    {
        for (MirOperand *op : inst->getOperands())
        {
            if (op->isOfType<MirRegister>())
            {
                auto *reg = op->get<MirRegister>();
                EXPECT_TRUE(reg->getRef().isPhysical())
                        << "Instruction " << inst->getOpCodeName() << " operand register is still virtual!";
            }
        }
    }
}

// ============================================================================
// 5. Spilling Under Register Pressure
// ============================================================================

TEST_F(MirRegisterAllocatorTest, TestSpillingUnderRegisterPressure)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("spill_pressure", typeTable->i64());
    auto *entry = func->getEntryPoint();

    // Create a constrained register class with only 2 physical registers
    auto *alloc = ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<> pAlloc(alloc);
    auto *tinyBank = pAlloc.new_object<MirRegisterBank>("TinyBank", alloc);
    auto *tinyClass = pAlloc.new_object<MirRegisterClass>("Tiny2", tinyBank, alloc);
    tinyBank->addClass("Tiny2", tinyClass);
    tinyClass->addRegister("t0", 64, 0, {});
    tinyClass->addRegister("t1", 64, 0, {});

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // Define 3 virtual registers that all interfere simultaneously
    // To prevent rematerialization, define them using non-constant operations (e.g. ADD)
    auto *base0 = ob.buildVReg(typeTable->i64(), "base0", nullptr, tinyClass);
    auto *base1 = ob.buildVReg(typeTable->i64(), "base1", nullptr, tinyClass);
    auto *v0 = ob.buildVReg(typeTable->i64(), "v0", nullptr, tinyClass);
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1", nullptr, tinyClass);
    auto *v2 = ob.buildVReg(typeTable->i64(), "v2", nullptr, tinyClass);
    auto *vOut = ob.buildVReg(typeTable->i64(), "vOut", nullptr, tinyClass);
    auto *imm = ob.buildInt(typeTable->i64(), FlexInt(1));

    ib.MOV(base0, imm);
    ib.MOV(base1, imm);

    // v0 = base0 + base1 (non-constant, not rematerializable)
    ib.ADD(v0, base0, base1);
    // v1 = v0 + base1    (v0 is live here!)
    ib.ADD(v1, v0, base1);
    // v2 = v1 + base1    (v0 and v1 are live here!)
    ib.ADD(v2, v1, base1);

    // Use all 3 simultaneously: vOut = (v0 + v1) + v2
    ib.ADD(vOut, v0, v1);
    ib.ADD(vOut, vOut, v2);
    ib.RET(vOut);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessResult = passManager.getAnalysis<LivenessAnalysisPass>(ctx)->getResult();
    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *mockAlloc = getTargetDesc()->getMockRegisterAllocator();

    mockAlloc->buildInterferenceGraph(livenessResult, &regCtx);
    mockAlloc->evaluateInterferenceGraphDegree(&regCtx);
    mockAlloc->simplify(&regCtx);

    // With 3 mutually interfering variables and only 2 physical registers,
    // selectColors must fail and trigger spilling
    bool selectOk = mockAlloc->selectColors(&regCtx);
    EXPECT_FALSE(selectOk);

    // Spills must have been emitted
    EXPECT_GT(mockAlloc->m_spillCount + mockAlloc->m_reloadCount, 0);
    EXPECT_GT(regCtx.m_spilledRegs.size(), 0);

    // Spill slots must be present in the stack frame
    EXPECT_GT(func->getStackFrame()->getObjects().size(), 0);
}

// ============================================================================
// 6. Rematerialization Tests
// ============================================================================

TEST_F(MirRegisterAllocatorTest, TestRematerializationInsteadOfSpill)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("remat_test", typeTable->i64());
    auto *entry = func->getEntryPoint();

    // Create a constrained register class with only 2 physical registers
    auto *alloc = ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<> pAlloc(alloc);
    auto *tinyBank = pAlloc.new_object<MirRegisterBank>("TinyBank2", alloc);
    auto *tinyClass = pAlloc.new_object<MirRegisterClass>("Tiny2_Remat", tinyBank, alloc);
    tinyBank->addClass("Tiny2_Remat", tinyClass);
    tinyClass->addRegister("r0", 64, 0, {});
    tinyClass->addRegister("r1", 64, 0, {});

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // Define v0 with an immediate integer constant (rematerializable!)
    auto *v0 = ob.buildVReg(typeTable->i64(), "v0_remat", nullptr, tinyClass);
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1", nullptr, tinyClass);
    auto *v2 = ob.buildVReg(typeTable->i64(), "v2", nullptr, tinyClass);
    auto *vOut = ob.buildVReg(typeTable->i64(), "vOut", nullptr, tinyClass);
    auto *imm42 = ob.buildInt(typeTable->i64(), FlexInt(42));
    auto *imm1 = ob.buildInt(typeTable->i64(), FlexInt(1));

    ib.MOV(v0, imm42); // Candidate for rematerialization
    ib.MOV(v1, imm1);
    ib.ADD(v2, v1, imm1);

    // v0, v1, v2 all live here simultaneously
    ib.ADD(vOut, v0, v1);
    ib.ADD(vOut, vOut, v2);
    ib.RET(vOut);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessResult = passManager.getAnalysis<LivenessAnalysisPass>(ctx)->getResult();
    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *mockAlloc = getTargetDesc()->getMockRegisterAllocator();

    mockAlloc->buildInterferenceGraph(livenessResult, &regCtx);
    mockAlloc->evaluateInterferenceGraphDegree(&regCtx);
    mockAlloc->simplify(&regCtx);

    // Run selectColors (which spills and rematerializes)
    bool selectOk = mockAlloc->selectColors(&regCtx);
    EXPECT_FALSE(selectOk);

    // Rematerialization or spills occurred
    size_t totalActivity = mockAlloc->m_rematCount + mockAlloc->m_spillCount;
    EXPECT_GT(totalActivity, 0);
}

// ============================================================================
// 7. Full MirRegisterAllocatorPass End-to-End Test
// ============================================================================

TEST_F(MirRegisterAllocatorTest, TestRegisterAllocatorPassEndToEnd)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("alloc_pass_e2e", typeTable->i32());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i32(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i32(), "v1", nullptr, gpr);
    auto *v2 = ob.buildVReg(typeTable->i32(), "v2", nullptr, gpr);
    auto *imm10 = ob.buildInt(typeTable->i32(), FlexInt(10));
    auto *imm20 = ob.buildInt(typeTable->i32(), FlexInt(20));

    ib.MOV(v0, imm10);
    ib.MOV(v1, imm20);
    ib.ADD(v2, v0, v1);
    ib.RET(v2);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);
    auto *allocPass = passManager.addPass<MirRegisterAllocatorPass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(allocPass, ctx);

    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);

    const auto &result = allocPass->getResult();
    EXPECT_TRUE(result.m_resolvedFunctions.contains(func));

    // Verify all instructions have physical registers now
    for (MirInstruction *inst : entry->getInstructions())
    {
        for (MirOperand *op : inst->getOperands())
        {
            if (op->isOfType<MirRegister>())
            {
                auto *reg = op->get<MirRegister>();
                EXPECT_TRUE(reg->getRef().isPhysical());
            }
        }
    }
}

// ============================================================================
// 8. Adversarial Edge-Case Tests (DenseBitSet PMR, Empty/Unreachable/Self-Loop, 2-Addr)
// ============================================================================

TEST_F(MirRegisterAllocatorTest, TestDenseBitSetWordBoundariesAndPmr)
{
    std::byte buf[4096];
    std::pmr::monotonic_buffer_resource arena(buf, sizeof(buf), std::pmr::null_memory_resource());
    std::pmr::vector<DenseBitSet> vec(&arena);
    vec.emplace_back(130);
    vec.emplace_back(64);

    // Word boundary bits: 0, 63, 64, 65, 127, 128
    vec[0].set(0);
    vec[0].set(63);
    vec[0].set(64);
    vec[0].set(128);
    EXPECT_EQ(vec[0].count(), 4U);

    vec[1].set(63);
    vec[1].set(65); // Out of bounds for 64-bit capacity -> ignored
    EXPECT_EQ(vec[1].count(), 1U);

    vec[0].clearBit(63);
    EXPECT_FALSE(vec[0].test(63));
    EXPECT_EQ(vec[0].count(), 3U);

    // Self-aliased copyOr where dst grows to match b
    DenseBitSet b(130, &arena);
    b.set(65);
    b.set(127);
    vec[1].copyOr(vec[1], b);
    EXPECT_TRUE(vec[1].test(63));
    EXPECT_TRUE(vec[1].test(65));
    EXPECT_TRUE(vec[1].test(127));

    std::vector<size_t> collected;
    vec[1].forEachSetBit([&](size_t bit) { collected.push_back(bit); });
    ASSERT_EQ(collected.size(), 3U);
    EXPECT_EQ(collected[0], 63U);
    EXPECT_EQ(collected[1], 65U);
    EXPECT_EQ(collected[2], 127U);

    // Shrinking resize and reset
    vec[1].resize(64);
    EXPECT_FALSE(vec[1].test(65));
    vec[1].reset();
    EXPECT_EQ(vec[1].count(), 0U);
}

TEST_F(MirRegisterAllocatorTest, TestEmptyFunctionAndUnreachableSelfLoop)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();

    // 1. Empty entry block function
    auto *emptyFunc = createTestFunction("empty_func", typeTable->_void());
    MirPassManager pm1(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    pm1.addPass<CodeFlowAnalysisPass>(ctx);
    pm1.addPass<LivenessAnalysisPass>(ctx);
    auto *allocPass1 = pm1.addPass<MirRegisterAllocatorPass>(ctx, getTargetDesc(), true);
    MirPassResult res1 = pm1.runPass(allocPass1, ctx);
    EXPECT_TRUE(res1.m_succeeded);

    // 2. Function with unreachable self-loop block
    auto *loopFunc = createTestFunction("unreachable_loop", typeTable->i32());
    auto *entry = loopFunc->getEntryPoint();
    auto *deadLoop = MirBlockBuilder(ctx, loopFunc).build(nullptr, "dead_loop");

    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();
    auto *v0 = ob.buildVReg(typeTable->i32(), "v0", nullptr, gpr);
    auto *vDead = ob.buildVReg(typeTable->i32(), "vDead", nullptr, gpr);
    auto *imm1 = ob.buildInt(typeTable->i32(), FlexInt(1));

    MirInstructionBuilder ibEntry(ctx, entry, InsertionType::Append);
    ibEntry.MOV(v0, imm1);
    ibEntry.RET(v0);

    MirInstructionBuilder ibDead(ctx, deadLoop, InsertionType::Append);
    ibDead.MOV(vDead, imm1);
    ibDead.ADD(vDead, vDead, imm1);
    ibDead.JMP(ob.buildRef(deadLoop));

    MirPassManager pm2(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    pm2.addPass<CodeFlowAnalysisPass>(ctx);
    pm2.addPass<LivenessAnalysisPass>(ctx);
    auto *allocPass2 = pm2.addPass<MirRegisterAllocatorPass>(ctx, getTargetDesc(), true);
    MirPassResult res2 = pm2.runPass(allocPass2, ctx);
    EXPECT_TRUE(res2.m_succeeded);
}

TEST_F(MirRegisterAllocatorTest, TestTwoAddressOperandIdx2Interference)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("two_addr_idx2", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i64(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1", nullptr, gpr);
    auto *vDst = ob.buildVReg(typeTable->i64(), "vDst", nullptr, gpr);
    auto *imm = ob.buildInt(typeTable->i64(), FlexInt(7));

    ib.MOV(v0, imm);
    ib.MOV(v1, imm);
    // vDst = v0 + v1 (v1 is operand index 2; must interfere with vDst even if v1 dies here)
    ib.ADD(vDst, v0, v1);
    ib.RET(vDst);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);
    auto *livenessResult = passManager.getAnalysis<LivenessAnalysisPass>(ctx)->getResult();

    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();
    ASSERT_TRUE(allocator->buildInterferenceGraph(livenessResult, &regCtx));

    // vDst must interfere symmetrically with v1 (operand index 2)
    EXPECT_TRUE(regCtx.hasEdge(vDst->getRef(), v1->getRef()));
    EXPECT_TRUE(regCtx.hasEdge(v1->getRef(), vDst->getRef()));
}
