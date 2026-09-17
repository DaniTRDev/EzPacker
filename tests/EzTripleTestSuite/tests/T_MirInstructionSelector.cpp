#include "EzTripleTestSuite.h"
#include "Instruction/MirInstruction.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"
#include "Operand/MirOperandBuilder.h"

class MirInstructionSelectorTest : public EzTripleTestSuite
{
};

TEST_F(MirInstructionSelectorTest, TestInstructionSelectionDispatch)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("isel_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *rd = ob.buildVReg(typeTable->i32(), "rd");
    MirRegister *rs1 = ob.buildVReg(typeTable->i32(), "rs1");
    MirRegister *rs2 = ob.buildVReg(typeTable->i32(), "rs2");

    ib.ADD(rd, rs1, rs2);
    ib.SUB(rd, rs1, rs2);

    EXPECT_EQ(block->getInstructions().size(), 2);

    auto *isel = getTargetDesc()->getInstructionSelector();
    bool ok = isel->selectBlock(ctx, block);
    EXPECT_TRUE(ok);

    auto *mockIsel = getTargetDesc()->getMockInstructionSelector();
    EXPECT_EQ(mockIsel->m_selectedCount, 2);
}

TEST_F(MirInstructionSelectorTest, TestInstructionSelectorPass)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("isel_pass_test", typeTable->i32());
    auto *block0 = func->getEntryPoint();
    auto *block1 = createBlock(func, "b1");

    MirInstructionBuilder ib0(ctx, block0, InsertionType::Append);
    MirInstructionBuilder ib1(ctx, block1, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *rd = ob.buildVReg(typeTable->i32(), "r");
    ib0.ADD(rd, rd, rd);
    ib1.MUL(rd, rd, rd);

    MirInstructionSelectorPass pass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto res = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(res.m_succeeded);
    EXPECT_TRUE(res.m_executed);

    auto *mockIsel = getTargetDesc()->getMockInstructionSelector();
    EXPECT_EQ(mockIsel->m_selectedCount, 2);
}

TEST_F(MirInstructionSelectorTest, TestMemoryFold)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("isel_fold_test", typeTable->i64());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *ptrReg = ob.buildVReg(typeTable->getPtr(typeTable->i64()), "ptr");
    MirRegister *rd = ob.buildVReg(typeTable->i64(), "rd");
    MirRegister *rs1 = ob.buildVReg(typeTable->i64(), "rs1");
    MirRegister *tmp = ob.buildVReg(typeTable->i64(), "tmp");
    MirMemory *memOp = ob.buildMem(typeTable->i64(), ptrReg, FlexInt(16));

    ib.LOAD(tmp, memOp);
    ib.ADD(rd, rs1, tmp);

    EXPECT_EQ(block->getInstructions().size(), 2);

    auto *isel = getTargetDesc()->getInstructionSelector();
    auto *mockIsel = getTargetDesc()->getMockInstructionSelector();
    mockIsel->m_selectedCount = 0;
    mockIsel->m_foldedCount = 0;

    bool ok = isel->selectBlock(ctx, block);
    EXPECT_TRUE(ok);
    EXPECT_EQ(mockIsel->m_foldedCount, 1);
    EXPECT_EQ(block->getInstructions().size(), 1);

    MirInstruction *selectedInst = block->front();
    ASSERT_NE(selectedInst, nullptr);
    EXPECT_TRUE(selectedInst->isSelected());
    EXPECT_STREQ(selectedInst->getTargetDesc()->getName(), "ADD64rm");

    // Virtual registers must now be constrained to GPR64
    EXPECT_EQ(rd->getRegClass(), getTargetDesc()->getGprClass());
    EXPECT_EQ(rs1->getRegClass(), getTargetDesc()->getGprClass());
    EXPECT_EQ(ptrReg->getRegClass(), getTargetDesc()->getGprClass());
}

TEST_F(MirInstructionSelectorTest, TestMultiUseNoFold)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("isel_multi_use_test", typeTable->i64());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *ptrReg = ob.buildVReg(typeTable->getPtr(typeTable->i64()), "ptr");
    MirRegister *rd = ob.buildVReg(typeTable->i64(), "rd");
    MirRegister *rd2 = ob.buildVReg(typeTable->i64(), "rd2");
    MirRegister *rs1 = ob.buildVReg(typeTable->i64(), "rs1");
    MirRegister *tmp = ob.buildVReg(typeTable->i64(), "tmp");
    MirMemory *memOp = ob.buildMem(typeTable->i64(), ptrReg, FlexInt(8));

    ib.LOAD(tmp, memOp);
    ib.ADD(rd, rs1, tmp);
    ib.SUB(rd2, rs1, tmp); // Second use of tmp prevents folding

    auto *isel = getTargetDesc()->getInstructionSelector();
    auto *mockIsel = getTargetDesc()->getMockInstructionSelector();
    mockIsel->m_selectedCount = 0;
    mockIsel->m_foldedCount = 0;

    bool ok = isel->selectBlock(ctx, block);
    EXPECT_TRUE(ok);
    EXPECT_EQ(mockIsel->m_foldedCount, 0);
    EXPECT_EQ(block->getInstructions().size(), 3);
}

TEST_F(MirInstructionSelectorTest, TestInterveningStoreNoFold)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("isel_intervening_store_test", typeTable->i64());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *ptrReg = ob.buildVReg(typeTable->getPtr(typeTable->i64()), "ptr");
    MirRegister *rd = ob.buildVReg(typeTable->i64(), "rd");
    MirRegister *rs1 = ob.buildVReg(typeTable->i64(), "rs1");
    MirRegister *val = ob.buildVReg(typeTable->i64(), "val");
    MirRegister *tmp = ob.buildVReg(typeTable->i64(), "tmp");
    MirMemory *memOp1 = ob.buildMem(typeTable->i64(), ptrReg, FlexInt(0));
    MirMemory *memOp2 = ob.buildMem(typeTable->i64(), ptrReg, FlexInt(8));

    ib.LOAD(tmp, memOp1);
    ib.STORE(memOp2, val); // Intervening store hazard prevents load folding
    ib.ADD(rd, rs1, tmp);

    auto *isel = getTargetDesc()->getInstructionSelector();
    auto *mockIsel = getTargetDesc()->getMockInstructionSelector();
    mockIsel->m_selectedCount = 0;
    mockIsel->m_foldedCount = 0;

    bool ok = isel->selectBlock(ctx, block);
    EXPECT_TRUE(ok);
    EXPECT_EQ(mockIsel->m_foldedCount, 0);
    EXPECT_EQ(block->getInstructions().size(), 3);
}

#include "RegisterAllocator/MirRegisterAllocatorPass.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"

TEST_F(MirInstructionSelectorTest, TestEndToEndWithRegisterAllocator)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("isel_e2e_ra_test", typeTable->i64());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *v0 = ob.buildVReg(typeTable->i64(), "v0");
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1");
    auto *v2 = ob.buildVReg(typeTable->i64(), "v2");
    auto *imm10 = ob.buildInt(typeTable->i64(), FlexInt(10));
    auto *imm20 = ob.buildInt(typeTable->i64(), FlexInt(20));

    ib.MOV(v0, imm10);
    ib.MOV(v1, imm20);
    ib.ADD(v2, v0, v1);
    ib.RET(v2);

    // Run ISel Pass
    MirInstructionSelectorPass iselPass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto iselRes = iselPass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(iselRes.m_succeeded);

    // Verify all instructions are selected and all vregs constrained
    for (MirInstruction *inst : block->getInstructions())
    {
        EXPECT_TRUE(inst->isSelected());
        for (size_t i = 0; i < inst->getOperandCount(); ++i)
        {
            if (auto *reg = inst->getOperand(i)->get<MirRegister>())
            {
                if (reg->isVirtual())
                {
                    EXPECT_NE(reg->getRegClass(), nullptr);
                }
            }
        }
    }

    // Run Register Allocator Pass on the ISel output
    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    MirRegisterAllocatorPass regAllocPass(ctx, getTargetDesc());
    auto regRes = regAllocPass.run(funcList.begin(), &passManager);
    EXPECT_TRUE(regRes.m_succeeded);
}

#include "FrameLowerer/MirFrameLowererPass.h"
#include "Function/MirFunctionStackFrame.h"

TEST_F(MirInstructionSelectorTest, TestFullPipelineAndInvariantAudit)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("full_pipeline_audit", typeTable->i64());
    auto *block = func->getEntryPoint();

    // Create a local stack frame object (simulating local variable)
    auto *stackObj = func->getStackFrame()->createStaticStackObj(typeTable->i64());
    ASSERT_NE(stackObj, nullptr);

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *ptrReg = ob.buildVReg(typeTable->getPtr(typeTable->i64()), "ptr");
    auto *v0 = ob.buildVReg(typeTable->i64(), "v0");
    auto *v1 = ob.buildVReg(typeTable->i64(), "v1");
    auto *v2 = ob.buildVReg(typeTable->i64(), "v2");
    auto *memOp = ob.buildMem(typeTable->i64(), ptrReg, FlexInt(8));

    // Pattern with load fold:
    // v0 = LOAD memOp
    // v1 = MOV 42
    // v2 = ADD v1, v0  --> folded into ADD64rm v2, v1, memOp
    // RET v2
    ib.LOAD(v0, memOp);
    ib.MOV(v1, ob.buildInt(typeTable->i64(), FlexInt(42)));
    ib.ADD(v2, v1, v0);
    ib.RET(v2);

    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    // 1. Run Instruction Selector Pass
    MirInstructionSelectorPass iselPass(ctx, getTargetDesc());
    auto iselRes = iselPass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(iselRes.m_succeeded);

    // 2. Invariant Audit:
    // Invariant 1: Exactly 0 generic opcodes remain in any basic block
    // Invariant 2: Exactly 0 virtual registers have null register class
    // Invariant 3: All MirMemory operands have valid base and legal displacements
    size_t instCount = 0;
    for (MirInstruction *inst : block->getInstructions())
    {
        instCount++;
        // Invariant 1: Must be selected target instruction
        EXPECT_TRUE(inst->isSelected());
        EXPECT_NE(inst->getTargetDesc(), nullptr);

        for (size_t opIdx = 0; opIdx < inst->getOperandCount(); ++opIdx)
        {
            MirOperand *op = inst->getOperand(opIdx);
            if (auto *reg = op->get<MirRegister>())
            {
                // Invariant 2: Virtual registers must have a valid register class
                if (reg->isVirtual())
                {
                    EXPECT_NE(reg->getRegClass(), nullptr);
                }
            }
            else if (auto *mem = op->get<MirMemory>())
            {
                // Invariant 3: Base register must be present and have GPR class
                EXPECT_NE(mem->getBase(), nullptr);
                EXPECT_NE(mem->getBase()->getRegClass(), nullptr);
                EXPECT_NE(mem->getDisplacement(), nullptr);
            }
        }
    }
    EXPECT_GT(instCount, 0);

    // 3. Run Register Allocator Pass on selected target instructions
    MirPassManager passManager(ctx->getDiagCollector(), ctx->getGlobalAllocator());
    passManager.addPass<CodeFlowAnalysisPass>(ctx);
    passManager.addPass<LivenessAnalysisPass>(ctx);

    MirRegisterAllocatorPass regAllocPass(ctx, getTargetDesc());
    auto regRes = regAllocPass.run(funcList.begin(), &passManager);
    EXPECT_TRUE(regRes.m_succeeded);

    // 4. Run Frame Lowerer Pass
    MirFrameLowererPass framePass(ctx, getTargetDesc());
    auto frameRes = framePass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(frameRes.m_succeeded);

    auto *mockLowerer = getTargetDesc()->getMockFrameLowerer();
    EXPECT_TRUE(mockLowerer->m_prologueInserted);
    EXPECT_TRUE(mockLowerer->m_epilogueInserted);
}

TEST_F(MirInstructionSelectorTest, TestSibAddressingModeMatching)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("sib_match_test", typeTable->i64());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *baseReg = ob.buildVReg(typeTable->getPtr(typeTable->i64()), "base");
    auto *idxReg = ob.buildVReg(typeTable->i64(), "idx");
    auto *scaledIdx = ob.buildVReg(typeTable->i64(), "scaledIdx");
    auto *addr1 = ob.buildVReg(typeTable->getPtr(typeTable->i64()), "addr1");
    auto *addr2 = ob.buildVReg(typeTable->getPtr(typeTable->i64()), "addr2");
    auto *val = ob.buildVReg(typeTable->i64(), "val");

    // scaledIdx = SHL idx, 2   (scale = 4)
    ib.SHL(scaledIdx, idxReg, ob.buildInt(typeTable->i64(), FlexInt(2)));
    // addr1 = ADD base, scaledIdx
    ib.ADD(addr1, baseReg, scaledIdx);
    // addr2 = ADD addr1, 64   (disp = 64)
    ib.ADD(addr2, addr1, ob.buildInt(typeTable->i64(), FlexInt(64)));
    // val = LOAD addr2
    ib.LOAD(val, addr2);

    EXPECT_EQ(block->getInstructions().size(), 4);

    // Test X86AddressingModeMatcher standalone
    X86AddressingModeMatcher matcher(getTargetDesc()->getInstructionSelector());
    MatchedAddressingMode mode;
    bool matchOk = matcher.matchAddress(ctx, addr2, mode);
    EXPECT_TRUE(matchOk);
    EXPECT_EQ(mode.m_base, baseReg);
    EXPECT_EQ(mode.m_index, idxReg);
    EXPECT_EQ(mode.m_scale, 4);
    EXPECT_EQ(mode.m_disp, 64);
    EXPECT_EQ(mode.m_foldedInstructions.size(), 3);

    // Test integrated selectBlock folding
    auto *mockIsel = getTargetDesc()->getMockInstructionSelector();
    mockIsel->m_selectedCount = 0;
    mockIsel->m_foldedCount = 0;

    auto *isel = getTargetDesc()->getInstructionSelector();
    bool selectOk = isel->selectBlock(ctx, block);
    EXPECT_TRUE(selectOk);
    EXPECT_EQ(mockIsel->m_foldedCount, 3);
    EXPECT_EQ(block->getInstructions().size(), 1);

    MirInstruction *selectedLoad = block->front();
    ASSERT_NE(selectedLoad, nullptr);
    EXPECT_TRUE(selectedLoad->isSelected());
    EXPECT_STREQ(selectedLoad->getTargetDesc()->getName(), "LOAD64");

    auto *memOp = selectedLoad->getOpAs<MirMemory>(1);
    ASSERT_NE(memOp, nullptr);
    EXPECT_EQ(memOp->getBase(), baseReg);
    EXPECT_EQ(memOp->getIndex(), idxReg);
    EXPECT_EQ(memOp->getScale(), 4);
    ASSERT_NE(memOp->getDisplacement(), nullptr);
    EXPECT_EQ(memOp->getDisplacement()->getValue().getI64(), 64);

    // Register class invariant: base and index must have GPR class assigned
    EXPECT_EQ(baseReg->getRegClass(), getTargetDesc()->getGprClass());
    EXPECT_EQ(idxReg->getRegClass(), getTargetDesc()->getGprClass());
    EXPECT_EQ(val->getRegClass(), getTargetDesc()->getGprClass());
}


