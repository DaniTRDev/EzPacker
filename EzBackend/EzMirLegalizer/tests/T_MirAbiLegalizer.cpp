#include <gtest/gtest.h>
#include <EzMirLegalizerCommon.h>
#include <MirAbiLegalizerPass/MirAbiLegalizerPass.h>
#include <MirTypeLegalizer/MirTypeLegalizerPass.h>
#include <MirLegalizerContext.h>
#include <ABIDesc.h>
#include <EzMir.h>
#include <memory>

class Mips64Abi : public ABIDesc
{
  public:
    ArgLocation getArgLoc(size_t id) const override
    {
        if (id == 1)
        {
            ArgLocation loc;
            loc.setLoc(PhysicalRegLocation{ 20, 32 });

            return loc;
        }
        else
        {
            ArgLocation loc;
            loc.setLoc(PhysicalRegLocation{ 21, 32 });

            return loc;
        }
    }
};

class MirAbiLegalizerTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;
    Mips64Abi abi;
    std::shared_ptr<MirLegalizerContext> ctx;
    MirPassManager *pm;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());
        pm = new MirPassManager();
        ctx = std::make_shared<MirLegalizerContext>(ec, sm);

        // Setup a 32-bit ABI (4 bytes)
        abi.setRegSizeInBits(32);
        abi.setEndianness(LittleEndian);
        abi.setStackReg(1);   // e.g. $sp
        abi.setStackFrame(2); // e.g. $fp

        ctx->setEmitter(emitter);
        ctx->setAbiDesc(&abi);
        ctx->setPassManager(pm);

        // Register passes
        pm->addPass<MirTypeLegalizerPass>(ctx.get());
        pm->addPass<MirAbiLegalizerPass>(ctx.get());
    }

    void TearDown() override
    {
        delete emitter;
        delete pm;
    }
};

TEST_F(MirAbiLegalizerTests, LegalizeReturnToPhysicalRegister)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    // Return a 32-bit value. ABI says this should be in register $v0 (id=10)
    ArgLocation loc;
    loc.setLoc(PhysicalRegLocation{ 10, 32 });

    abi.setReturnValueLoc(loc);

    MirRegister retVal = emitter->createVirtualRegister(4);
    emitter->emitRET(MirOperand(retVal));

    pm->run(func);

    // Should be: MOV $v0, retVal; RET $v0
    EXPECT_EQ(block->getInstructions()->m_numElems, 2u);

    auto it = block->getInstructions()->begin();
    MirInstruction *movInstr = *it;
    EXPECT_EQ(movInstr->getOpCode(), MirInstructionOpCode::MOV);

    auto *movOps = movInstr->getOperands();
    EXPECT_EQ(movOps->get<MirOperand>(0)->getRegister()->m_id, 10u); // $v0
    EXPECT_EQ(movOps->get<MirOperand>(0)->getRegister()->m_virtual, false);

    ++it;
    MirInstruction *retInstr = *it;
    EXPECT_EQ(retInstr->getOpCode(), MirInstructionOpCode::RET);
    EXPECT_EQ(retInstr->getOperands()->get<MirOperand>(0)->getRegister()->m_id, 10u);

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirAbiLegalizerTests, LegalizeReturnToStack)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    // Return a 64-bit value. ABI says this is returned on the stack at offset 8 from frame pointer.
    ArgLocation loc;
    loc.setLoc(StackLocation{ 8 });

    abi.setReturnValueLoc(loc);

    MirRegister retVal = emitter->createVirtualRegister(8);
    emitter->emitRET(MirOperand(retVal));

    pm->run(func);

    // Type legalizer: RET val64 -> RET chunk0, chunk1
    // ABI legalizer: STORE $fp, 8, chunk0; STORE $fp, 12, chunk1; RET

    bool hasStore = false;
    for (auto instr : *block->getInstructions())
    {
        if (instr->getOpCode() == MirInstructionOpCode::STORE)
        {
            hasStore = true;
            auto *ops = instr->getOperands();
            EXPECT_EQ(ops->get<MirOperand>(0)->getRegister()->m_virtual, false);
            EXPECT_EQ(ops->get<MirOperand>(0)->getRegister()->m_id, 2u); // $fp
        }
    }
    EXPECT_TRUE(hasStore);

    // The final instruction should be a RET with no operands
    auto lastInstr = block->getInstructions()->get<MirInstruction>(block->getInstructions()->m_numElems - 1);
    EXPECT_EQ(lastInstr->getOpCode(), MirInstructionOpCode::RET);
    EXPECT_EQ(lastInstr->getOperands()->m_numElems, 0u);

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirAbiLegalizerTests, LegalizeCallArgumentsToPhysicalRegisters)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    // ABI: arg1 in $a0 (id=20), arg2 in $a1 (id=21)

    MirRegister arg1 = emitter->createVirtualRegister(4);
    MirRegister arg2 = emitter->createVirtualRegister(4);
    MirFunction *callee = emitterCtx->createFunction(1);
    MirReference calleeRef = emitterCtx->createReference(callee->getEntryPoint());

    emitter->emitCALL(MirOperand(calleeRef), MirOperand(arg1), MirOperand(arg2));

    pm->run(func);

    // MOV $a0, arg1; MOV $a1, arg2; CALL callee
    EXPECT_EQ(block->getInstructions()->m_numElems, 3u);

    auto it = block->getInstructions()->begin();
    MirInstruction *mov1 = *it;
    EXPECT_EQ(mov1->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(mov1->getOperands()->get<MirOperand>(0)->getRegister()->m_id, 20u);

    ++it;
    MirInstruction *mov2 = *it;
    EXPECT_EQ(mov2->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(mov2->getOperands()->get<MirOperand>(0)->getRegister()->m_id, 21u);

    ++it;
    MirInstruction *callInstr = *it;
    EXPECT_EQ(callInstr->getOpCode(), MirInstructionOpCode::CALL);
    EXPECT_EQ(callInstr->getOperands()->m_numElems, 1u); // Args moved out

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirAbiLegalizerTests, LegalizeSplitReturn)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    // Return a 64-bit value split across two registers $v0 (10) and $v1 (11)
    ArgLocation loc;
    std::vector<PhysicalRegLocation> splitRegs = { { 10, 32 }, { 11, 32 } };

    loc.setLoc(SplitLocation{ splitRegs });
    abi.setReturnValueLoc(loc);

    MirRegister retVal = emitter->createVirtualRegister(8);
    emitter->emitRET(MirOperand(retVal));

    pm->run(func);

    // Type Legalizer: RET retVal64 -> RET chunk0, chunk1
    // ABI Legalizer: MOV $v0, chunk0; MOV $v1, chunk1; RET $v0, $v1
    EXPECT_EQ(block->getInstructions()->m_numElems, 3u);

    auto it = block->getInstructions()->begin();
    MirInstruction *mov1 = *it;
    EXPECT_EQ(mov1->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(mov1->getOperands()->get<MirOperand>(0)->getRegister()->m_id, 10u);

    ++it;
    MirInstruction *mov2 = *it;
    EXPECT_EQ(mov2->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(mov2->getOperands()->get<MirOperand>(0)->getRegister()->m_id, 11u);

    ++it;
    MirInstruction *retInstr = *it;
    EXPECT_EQ(retInstr->getOpCode(), MirInstructionOpCode::RET);
    EXPECT_EQ(retInstr->getOperands()->m_numElems, 2u);
    EXPECT_EQ(retInstr->getOperands()->get<MirOperand>(0)->getRegister()->m_id, 10u);
    EXPECT_EQ(retInstr->getOperands()->get<MirOperand>(1)->getRegister()->m_id, 11u);

    ec->endScope(ErrorAction::Discard);
}
