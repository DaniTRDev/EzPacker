#include "EzTripleTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "MirPasses/MirPassManager.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Passes/MirTargetPeepholePass.h"
#include "Type/MirTypeTable.h"

class MirTargetPeepholeTest : public EzTripleTestSuite
{
};

// ============================================================================
// 1. Machine Self-Move Elimination
// ============================================================================

TEST_F(MirTargetPeepholeTest, TestEliminateTargetSelfMove)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("self_move_elim", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *p0Desc = gpr->getRegs().begin()->second;
    MirOperand *p0Op1 = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    MirOperand *p0Op2 = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);

    auto *movDesc = getTargetDesc()->getDescMOV64rr();
    auto *retDesc = getTargetDesc()->getDescRET();

    // MOV64rr p0, p0
    // RET p0
    ib.buildTarget(movDesc, nullptr, { p0Op1, p0Op2 });
    ib.buildTarget(retDesc, nullptr, { p0Op1 });

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    auto *peepPass = passManager.addPass<MirTargetPeepholePass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(peepPass, ctx);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);
    EXPECT_EQ(peepPass->getMetrics().m_selfMovesEliminated, 1);

    // Only RET remains
    size_t count = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        count++;
    }
    EXPECT_EQ(count, 1);
}

// ============================================================================
// 2. Machine Reciprocal Move Elimination
// ============================================================================

TEST_F(MirTargetPeepholeTest, TestEliminateTargetReciprocalMove)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("reciprocal_move_elim", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto it = gpr->getRegs().begin();
    auto *p0Desc = it->second;
    ++it;
    auto *p1Desc = it->second;

    MirOperand *p0A = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    MirOperand *p1A = ob.buildPhysReg(typeTable->i64(), p1Desc->m_id, p1Desc->m_name, gpr);
    MirOperand *p1B = ob.buildPhysReg(typeTable->i64(), p1Desc->m_id, p1Desc->m_name, gpr);
    MirOperand *p0B = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);

    auto *movDesc = getTargetDesc()->getDescMOV64rr();
    auto *retDesc = getTargetDesc()->getDescRET();

    // MOV64rr p0, p1  (p0 = p1)
    // MOV64rr p1, p0  (p1 = p0, reciprocal no-op!)
    // RET p0
    ib.buildTarget(movDesc, nullptr, { p0A, p1A });
    ib.buildTarget(movDesc, nullptr, { p1B, p0B });
    ib.buildTarget(retDesc, nullptr, { p0A });

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    auto *peepPass = passManager.addPass<MirTargetPeepholePass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(peepPass, ctx);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);
    EXPECT_EQ(peepPass->getMetrics().m_reciprocalMovesEliminated, 1);

    // MOV64rr p0, p1 and RET p0 remain (count = 2)
    size_t count = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        count++;
    }
    EXPECT_EQ(count, 2);
}

// ============================================================================
// 3. Redundant Adjacent Jump Elimination
// ============================================================================

TEST_F(MirTargetPeepholeTest, TestEliminateAdjacentJump)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("adjacent_jump_elim", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirBlockBuilder bb(ctx, func);
    auto *nextBlock = bb.build(nullptr, "next_block");

    MirInstructionBuilder ibEntry(ctx, entry, InsertionType::Append);
    MirInstructionBuilder ibNext(ctx, nextBlock, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *p0Desc = gpr->getRegs().begin()->second;
    MirOperand *p0Op = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);

    // Entry jumps unconditionally to nextBlock
    ibEntry.JMP(ob.buildRef(nextBlock));
    // nextBlock returns p0
    ibNext.RET(p0Op);

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    auto *peepPass = passManager.addPass<MirTargetPeepholePass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(peepPass, ctx);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);
    EXPECT_EQ(peepPass->getMetrics().m_redundantJumpsEliminated, 1);

    // Entry block jump should be erased
    size_t count = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        count++;
    }
    EXPECT_EQ(count, 0);
}

// ============================================================================
// 4. Zero-Identity Arithmetic Elimination
// ============================================================================

TEST_F(MirTargetPeepholeTest, TestEliminateZeroIdentityArithmetic)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("zero_arith_elim", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto *p0Desc = gpr->getRegs().begin()->second;
    MirOperand *p0A = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    MirOperand *p0B = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    MirOperand *immZero = ob.buildInt(typeTable->i64(), FlexInt(0));

    auto *addDesc = getTargetDesc()->getDescADD64ri();
    auto *retDesc = getTargetDesc()->getDescRET();

    // ADD64ri p0, p0, 0
    // RET p0
    ib.buildTarget(addDesc, nullptr, { p0A, p0B, immZero });
    ib.buildTarget(retDesc, nullptr, { p0A });

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    auto *peepPass = passManager.addPass<MirTargetPeepholePass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(peepPass, ctx);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);
    EXPECT_EQ(peepPass->getMetrics().m_zeroIdentitiesEliminated, 1);

    // Only RET remains
    size_t count = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        count++;
    }
    EXPECT_EQ(count, 1);
}

// ============================================================================
// 5. Spill-Reload Forwarding (Redundant Load after Store)
// ============================================================================

TEST_F(MirTargetPeepholeTest, TestSpillReloadForwarding)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("spill_reload_fwd", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();
    auto regIt = gpr->getRegs().begin();
    auto *p0Desc = regIt->second;
    ++regIt;
    auto *baseDesc = regIt->second;

    MirOperand *p0A = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    MirOperand *p0B = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    auto *baseReg = ob.buildPhysReg(typeTable->getPtr(typeTable->i64()), baseDesc->m_id, baseDesc->m_name, gpr);

    MirOperand *mem1 = ob.buildMem(typeTable->i64(), baseReg, FlexInt(16));
    MirOperand *mem2 = ob.buildMem(typeTable->i64(), baseReg, FlexInt(16));

    auto *storeDesc = getTargetDesc()->getDescSTORE64();
    auto *loadDesc = getTargetDesc()->getDescLOAD64();
    auto *retDesc = getTargetDesc()->getDescRET();

    // STORE64 [base + 16], p0
    // LOAD64 p0, [base + 16]   (reloading p0 which already has the value!)
    // RET p0
    ib.buildTarget(storeDesc, nullptr, { mem1, p0A });
    ib.buildTarget(loadDesc, nullptr, { p0B, mem2 });
    ib.buildTarget(retDesc, nullptr, { p0A });

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    auto *peepPass = passManager.addPass<MirTargetPeepholePass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(peepPass, ctx);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);
    EXPECT_EQ(peepPass->getMetrics().m_spillReloadsForwarded, 1);

    // STORE and RET remain, LOAD eliminated
    size_t count = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        count++;
    }
    EXPECT_EQ(count, 2);
}

// ============================================================================
// 6. Redundant Consecutive Loads
// ============================================================================

TEST_F(MirTargetPeepholeTest, TestRedundantConsecutiveLoads)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("consecutive_loads", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();
    auto regIt = gpr->getRegs().begin();
    auto *p0Desc = regIt->second;
    ++regIt;
    auto *baseDesc = regIt->second;

    MirOperand *p0A = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    MirOperand *p0B = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    auto *baseReg = ob.buildPhysReg(typeTable->getPtr(typeTable->i64()), baseDesc->m_id, baseDesc->m_name, gpr);

    MirOperand *mem1 = ob.buildMem(typeTable->i64(), baseReg, FlexInt(8));
    MirOperand *mem2 = ob.buildMem(typeTable->i64(), baseReg, FlexInt(8));

    auto *loadDesc = getTargetDesc()->getDescLOAD64();
    auto *retDesc = getTargetDesc()->getDescRET();

    // LOAD64 p0, [base + 8]
    // LOAD64 p0, [base + 8]   (redundant consecutive load)
    // RET p0
    ib.buildTarget(loadDesc, nullptr, { p0A, mem1 });
    ib.buildTarget(loadDesc, nullptr, { p0B, mem2 });
    ib.buildTarget(retDesc, nullptr, { p0A });

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    auto *peepPass = passManager.addPass<MirTargetPeepholePass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(peepPass, ctx);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);
    EXPECT_EQ(peepPass->getMetrics().m_redundantLoadsEliminated, 1);

    // 1 LOAD and RET remain
    size_t count = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        count++;
    }
    EXPECT_EQ(count, 2);
}

// ============================================================================
// 7. Dead Store Elimination
// ============================================================================

TEST_F(MirTargetPeepholeTest, TestDeadStoreElimination)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("dead_store_elim", typeTable->i64());
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);
    auto *gpr = getTargetDesc()->getGprClass();

    auto it = gpr->getRegs().begin();
    auto *p0Desc = it->second;
    ++it;
    auto *p1Desc = it->second;
    ++it;
    auto *baseDesc = it->second;

    MirOperand *p0 = ob.buildPhysReg(typeTable->i64(), p0Desc->m_id, p0Desc->m_name, gpr);
    MirOperand *p1 = ob.buildPhysReg(typeTable->i64(), p1Desc->m_id, p1Desc->m_name, gpr);
    auto *baseReg = ob.buildPhysReg(typeTable->getPtr(typeTable->i64()), baseDesc->m_id, baseDesc->m_name, gpr);

    MirOperand *mem1 = ob.buildMem(typeTable->i64(), baseReg, FlexInt(24));
    MirOperand *mem2 = ob.buildMem(typeTable->i64(), baseReg, FlexInt(24));

    auto *storeDesc = getTargetDesc()->getDescSTORE64();
    auto *retDesc = getTargetDesc()->getDescRET();

    // STORE64 [base + 24], p0  (immediately overwritten without any read!)
    // STORE64 [base + 24], p1
    // RET p1
    ib.buildTarget(storeDesc, nullptr, { mem1, p0 });
    ib.buildTarget(storeDesc, nullptr, { mem2, p1 });
    ib.buildTarget(retDesc, nullptr, { p1 });

    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    auto *peepPass = passManager.addPass<MirTargetPeepholePass>(ctx, getTargetDesc());

    MirPassResult res = passManager.runPass(peepPass, ctx);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_modifiedMir);
    EXPECT_EQ(peepPass->getMetrics().m_deadStoresEliminated, 1);

    // 1 STORE and RET remain
    size_t count = 0;
    for (MirInstruction *inst : entry->getInstructions())
    {
        (void)inst;
        count++;
    }
    EXPECT_EQ(count, 2);
}
