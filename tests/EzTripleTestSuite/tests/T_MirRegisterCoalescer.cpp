#include "EzTripleTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
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
 * Fixture testing conservative register coalescing, affinity-biased coloring,
 * and redundant copy elimination in the graph-coloring register allocator.
 */
class MirRegisterCoalescerTest : public EzTripleTestSuite
{
  protected:
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
// 1. Virtual-to-Virtual Coalescing (Briggs' Criterion)
// ============================================================================

TEST_F(MirRegisterCoalescerTest, TestCoalesceVirtualToVirtual)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("coalesce_vreg_to_vreg", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i64(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1", nullptr, gpr);
    auto *imm42 = ob.buildInt(typeTable->i64(), FlexInt(42));

    // v0 = 42
    // v1 = v0 (copy)
    // RET v1
    ib.MOV(v0, imm42);
    ib.MOV(v1, v0);
    ib.RET(v1);

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

    // v0 and v1 do not interfere (v0 dies at the copy into v1)
    auto it0 = regCtx.m_iGraph.find(v0->getRef());
    ASSERT_NE(it0, regCtx.m_iGraph.end());
    EXPECT_FALSE(it0->second.contains(v1->getRef()));

    bool coalesced = allocator->coalesce(&regCtx);
    EXPECT_TRUE(coalesced);

    // Both v0 and v1 must share the same leader
    MirRegisterRef leader0 = regCtx.getCoalescedLeader(v0->getRef());
    MirRegisterRef leader1 = regCtx.getCoalescedLeader(v1->getRef());
    EXPECT_EQ(leader0, leader1);

    // Simplify, select colors, and rewrite
    allocator->evaluateInterferenceGraphDegree(&regCtx);
    EXPECT_TRUE(allocator->simplify(&regCtx));
    EXPECT_TRUE(allocator->selectColors(&regCtx));

    allocator->rewriteColors(&regCtx);

    // Redundant copy MOV v1, v0 should have been eliminated
    // Only MOV v0, 42 and RET v0 should remain in the entry block
    size_t instCount = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        instCount++;
    }
    EXPECT_EQ(instCount, 2);
}

// ============================================================================
// 2. Virtual-to-Physical Coalescing (George's Criterion)
// ============================================================================

TEST_F(MirRegisterCoalescerTest, TestCoalesceVirtualToPhysical)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("coalesce_vreg_to_preg", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    // Use physical register r0 from gpr
    ASSERT_FALSE(gpr->getRegs().empty());
    auto *p0Desc = gpr->getRegs().begin()->second;
    MirRegisterRef p0Ref = MirRegisterRef::preg(p0Desc);
    MirOperand *p0Op = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);

    auto *v0 = ob.buildVReg(typeTable->i64(), "v0", nullptr, gpr);
    MirOperand *v0Op = v0;

    // Build target move: MOV64rr v0, p0
    // RET v0
    auto *movDesc = getTargetDesc()->getDescMOV64rr();
    ASSERT_NE(movDesc, nullptr);
    auto *retDesc = getTargetDesc()->getDescRET();
    ASSERT_NE(retDesc, nullptr);

    ib.buildTarget(movDesc, nullptr, { v0Op, p0Op });
    ib.buildTarget(retDesc, nullptr, { v0Op });

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessPass = passManager.getAnalysis<LivenessAnalysisPass>(ctx);
    ASSERT_NE(livenessPass, nullptr);
    auto *livenessResult = livenessPass->getResult();

    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();
    ASSERT_NE(allocator, nullptr);

    bool buildOk = allocator->buildInterferenceGraph(livenessResult, &regCtx);
    EXPECT_TRUE(buildOk);

    bool coalesced = allocator->coalesce(&regCtx);
    EXPECT_TRUE(coalesced);

    // v0 must have coalesced into p0Ref
    EXPECT_EQ(regCtx.getCoalescedLeader(v0->getRef()), p0Ref);

    allocator->evaluateInterferenceGraphDegree(&regCtx);
    EXPECT_TRUE(allocator->simplify(&regCtx));
    EXPECT_TRUE(allocator->selectColors(&regCtx));

    allocator->rewriteColors(&regCtx);

    // Redundant MOV64rr p0, p0 must be eliminated; only RET remains
    size_t instCount = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        instCount++;
    }
    EXPECT_EQ(instCount, 1);
}

// ============================================================================
// 3. Interfering Registers Rejected from Coalescing
// ============================================================================

TEST_F(MirRegisterCoalescerTest, TestInterferingRegistersNotCoalesced)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("interfering_no_coalesce", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i64(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1", nullptr, gpr);
    auto *v2 = ob.buildVReg(typeTable->i64(), "v2", nullptr, gpr);
    auto *imm1 = ob.buildInt(typeTable->i64(), FlexInt(1));
    auto *imm2 = ob.buildInt(typeTable->i64(), FlexInt(2));

    // v0 = 1
    // v1 = 2
    // v2 = v0   <-- copy from v0 to v2, but v0 is still used later!
    // ADD v0, v0, v1
    // RET v2
    ib.MOV(v0, imm1);
    ib.MOV(v1, imm2);
    ib.MOV(v2, v0);
    ib.ADD(v0, v0, v1); // v0 remains live across the copy of v2!
    ib.RET(v2);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessPass = passManager.getAnalysis<LivenessAnalysisPass>(ctx);
    ASSERT_NE(livenessPass, nullptr);
    auto *livenessResult = livenessPass->getResult();

    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();
    ASSERT_NE(allocator, nullptr);

    EXPECT_TRUE(allocator->buildInterferenceGraph(livenessResult, &regCtx));

    // v0 and v2 must interfere because v0 is live-out of the MOV v2, v0 instruction
    auto itV0 = regCtx.m_iGraph.find(v0->getRef());
    ASSERT_NE(itV0, regCtx.m_iGraph.end());
    EXPECT_TRUE(itV0->second.contains(v2->getRef()));

    // Coalesce cannot merge v0 and v2
    allocator->coalesce(&regCtx);
    EXPECT_NE(regCtx.getCoalescedLeader(v0->getRef()), regCtx.getCoalescedLeader(v2->getRef()));
}

// ============================================================================
// 4. Two-Address Instruction Coalescing
// ============================================================================

TEST_F(MirRegisterCoalescerTest, TestTwoAddressCoalescing)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("two_address_coalesce", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i64(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1", nullptr, gpr);
    auto *v2 = ob.buildVReg(typeTable->i64(), "v2", nullptr, gpr);
    auto *imm10 = ob.buildInt(typeTable->i64(), FlexInt(10));
    auto *imm20 = ob.buildInt(typeTable->i64(), FlexInt(20));

    // v0 = 10
    // v1 = 20
    // ADD64rr v2, v0, v1  (v0 dies here, two-address candidate: v2 <-> v0)
    // RET v2
    auto *addDesc = getTargetDesc()->getDescADD64rr();
    ASSERT_NE(addDesc, nullptr);
    auto *retDesc = getTargetDesc()->getDescRET();
    ASSERT_NE(retDesc, nullptr);

    MirOperand *v0Op = v0;
    MirOperand *v1Op = v1;
    MirOperand *v2Op = v2;

    ib.MOV(v0, imm10);
    ib.MOV(v1, imm20);
    ib.buildTarget(addDesc, nullptr, { v2Op, v0Op, v1Op });
    ib.buildTarget(retDesc, nullptr, { v2Op });

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    auto *livenessPass = passManager.getAnalysis<LivenessAnalysisPass>(ctx);
    auto *livenessResult = livenessPass->getResult();

    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();
    ASSERT_NE(allocator, nullptr);

    EXPECT_TRUE(allocator->buildInterferenceGraph(livenessResult, &regCtx));

    bool coalesced = allocator->coalesce(&regCtx);
    EXPECT_TRUE(coalesced);

    // v2 and v0 must share the same coalesced leader
    EXPECT_EQ(regCtx.getCoalescedLeader(v2->getRef()), regCtx.getCoalescedLeader(v0->getRef()));
}

// ============================================================================
// 5. Affinity Biasing in Color Selection
// ============================================================================

TEST_F(MirRegisterCoalescerTest, TestAffinityBiasing)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("affinity_biasing", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i64(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1", nullptr, gpr);

    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();
    ASSERT_NE(allocator, nullptr);

    // Add both to interference graph without interfering with each other
    regCtx.m_iGraph[v0->getRef()];
    regCtx.m_iGraph[v1->getRef()];

    // Establish affinity between v0 and v1
    regCtx.m_affinity[v0->getRef()].push_back(v1->getRef());
    regCtx.m_affinity[v1->getRef()].push_back(v0->getRef());

    // Push both onto select stack
    allocator->evaluateInterferenceGraphDegree(&regCtx);
    regCtx.m_selectStack.push_back(v0->getRef());
    regCtx.m_selectStack.push_back(v1->getRef());

    // Select colors
    EXPECT_TRUE(allocator->selectColors(&regCtx));

    // Both should receive the identical physical register color due to affinity biasing
    auto col0 = regCtx.m_allocatedRegs[v0->getRef()];
    auto col1 = regCtx.m_allocatedRegs[v1->getRef()];
    EXPECT_TRUE(col0.isPhysical());
    EXPECT_TRUE(col1.isPhysical());
    EXPECT_EQ(col0, col1);
}

// ============================================================================
// 6. Redundant Copy Elimination
// ============================================================================

TEST_F(MirRegisterCoalescerTest, TestEliminateRedundantCopies)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("eliminate_redundant_copies", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    ASSERT_FALSE(gpr->getRegs().empty());
    auto *p0Desc = gpr->getRegs().begin()->second;
    MirOperand *p0Op1 = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    MirOperand *p0Op2 = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);

    auto *movDesc = getTargetDesc()->getDescMOV64rr();
    ASSERT_NE(movDesc, nullptr);
    auto *retDesc = getTargetDesc()->getDescRET();
    ASSERT_NE(retDesc, nullptr);

    // MOV64rr p0, p0  (identity move)
    // RET p0
    ib.buildTarget(movDesc, nullptr, { p0Op1, p0Op2 });
    ib.buildTarget(retDesc, nullptr, { p0Op1 });

    RegisterAllocatorCtx regCtx(ctx, func, getTargetDesc(), ctx->getGlobalAllocator());
    auto *allocator = getTargetDesc()->getRegisterAllocator();
    ASSERT_NE(allocator, nullptr);

    allocator->eliminateRedundantCopies(&regCtx);

    // The identity move must be deleted, leaving only RET
    size_t count = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        count++;
    }
    EXPECT_EQ(count, 1);
}

// ============================================================================
// 7. Full MirRegisterAllocatorPass End-to-End with Coalescing
// ============================================================================

TEST_F(MirRegisterCoalescerTest, TestRegisterAllocatorPassWithCoalescing)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("alloc_pass_coalesce_e2e", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *v0 = ob.buildVReg(typeTable->i64(), "v0", nullptr, gpr);
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1", nullptr, gpr);
    auto *v2 = ob.buildVReg(typeTable->i64(), "v2", nullptr, gpr);
    auto *imm10 = ob.buildInt(typeTable->i64(), FlexInt(10));
    auto *imm20 = ob.buildInt(typeTable->i64(), FlexInt(20));

    // v0 = 10
    // v1 = 20
    // v2 = v0        (copy)
    // ADD v2, v2, v1
    // RET v2
    ib.MOV(v0, imm10);
    ib.MOV(v1, imm20);
    ib.MOV(v2, v0);
    ib.ADD(v2, v2, v1);
    ib.RET(v2);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);
    auto *allocPass = passManager.addPass<MirRegisterAllocatorPass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(allocPass, ctx);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);

    // Verify all registers are physical
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

    // The copy MOV v2, v0 should have been coalesced and eliminated!
    // Original had 5 instructions: MOV, MOV, MOV(copy), ADD, RET.
    // After coalescing and eliminating the copy, exactly 4 instructions should remain!
    size_t instCount = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        instCount++;
    }
    EXPECT_EQ(instCount, 4);
}
