#include <gtest/gtest.h>
#include <EzMirLegalizerCommon.h>
#include <MirTypeLegalizer/MirTypeLegalizerPass.h>
#include <MirLegalizerContext.h>
#include <ABIDesc.h>
#include <EzMir.h>
#include <memory>

class Mips64Abi : public ABIDesc
{
  public:
    const ArgLocation &getArgLoc(size_t id) const override
    {
        // Not needed for the test.
        static ArgLocation test{};
        return test;
    }
};

class MirTypeLegalizerTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;
    std::shared_ptr<MirLegalizerContext> legalizerCtx;
    Mips64Abi abi;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());
        legalizerCtx = std::make_shared<MirLegalizerContext>(ec, sm);

        // Setup a 32-bit ABI (4 bytes)
        abi.setRegSizeInBits(32);
        abi.setEndianness(LittleEndian);

        legalizerCtx->setEmitter(emitter);
        legalizerCtx->setAbiDesc(&abi);
    }

    void TearDown() override { delete emitter; }
};

TEST_F(MirTypeLegalizerTests, NoChangesForLegalSizedRegisters)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1); // valid type id 1
    MirBlock *block = *func->getBlocks()->begin();
    emitterCtx->bindToBlock(block);

    // Create 32-bit (4 byte) registers which matches the target ABI
    MirRegister r1 = emitter->createVirtualRegister(4);
    MirRegister r2 = emitter->createVirtualRegister(4);
    emitter->emitMOV(MirOperand(r1), MirOperand(r2));

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_FALSE(changed);
    EXPECT_EQ(block->getInstructions()->m_numElems, 1u);
    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirTypeLegalizerTests, ExpandMovWithLargeRegister)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    // Create a 64-bit (8 bytes) register on our 32-bit ABI machine
    MirRegister dest = emitter->createVirtualRegister(8);
    MirRegister src = emitter->createVirtualRegister(8);
    emitter->emitMOV(MirOperand(dest), MirOperand(src));

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);
    // MOV [8-byte] -> MOV [4-byte], MOV [4-byte]
    EXPECT_EQ(block->getInstructions()->m_numElems, 2u);

    auto it = block->getInstructions()->begin();
    MirInstruction *firstInstr = *it;
    EXPECT_EQ(firstInstr->getOpCode(), MirInstructionOpCode::MOV);

    ++it;
    MirInstruction *secondInstr = *it;
    EXPECT_EQ(secondInstr->getOpCode(), MirInstructionOpCode::MOV);

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirTypeLegalizerTests, PromoteMovWithSmallRegister)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    // Create a 16-bit (2 bytes) register on our 32-bit ABI machine
    MirRegister dest = emitter->createVirtualRegister(2);
    MirRegister src = emitter->createVirtualRegister(2);
    emitter->emitMOV(MirOperand(dest), MirOperand(src));

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);
    // The instruction should be promoted, replacing operands to 4-bytes
    // The total instruction count should remain 1.
    EXPECT_EQ(block->getInstructions()->m_numElems, 1u);

    auto it = block->getInstructions()->begin();
    MirInstruction *newInstr = *it;
    EXPECT_EQ(newInstr->getOpCode(), MirInstructionOpCode::MOV);

    // Ensure the size of the promoted registers is now 4 bytes
    auto *ops = newInstr->getOperands();
    EXPECT_EQ(ops->m_numElems, 2u);
    EXPECT_EQ(ops->get<MirOperand>(0)->getRegister()->m_sizeInBytes, 4u);
    EXPECT_EQ(ops->get<MirOperand>(1)->getRegister()->m_sizeInBytes, 4u);

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirTypeLegalizerTests, ExpandAddProducesAddAndAdc)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    // Create a 64-bit (8 bytes) register on our 32-bit ABI machine
    MirRegister dest = emitter->createVirtualRegister(8);
    MirRegister src = emitter->createVirtualRegister(8);
    emitter->emitADD(MirOperand(dest), MirOperand(src));

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);
    EXPECT_EQ(block->getInstructions()->m_numElems, 2u);

    auto it = block->getInstructions()->begin();
    MirInstruction *addInstr = *it;
    EXPECT_EQ(addInstr->getOpCode(), MirInstructionOpCode::ADD);

    ++it;
    MirInstruction *adcInstr = *it;
    EXPECT_EQ(adcInstr->getOpCode(), MirInstructionOpCode::ADC); // Carry from first

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirTypeLegalizerTests, PromoteAddInjectsMask)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    // Create a 8-bit (1 byte) register on our 32-bit machine
    MirRegister dest = emitter->createVirtualRegister(1);
    MirRegister src = emitter->createVirtualRegister(1);
    emitter->emitADD(MirOperand(dest), MirOperand(src));

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);

    // Promote Math operation requires Overflow Mask, so ADD -> ADD, AND(Masking)
    EXPECT_EQ(block->getInstructions()->m_numElems, 2u);

    auto it = block->getInstructions()->begin();
    MirInstruction *addInstr = *it;
    EXPECT_EQ(addInstr->getOpCode(), MirInstructionOpCode::ADD);

    ++it;
    MirInstruction *maskInstr = *it;
    EXPECT_EQ(maskInstr->getOpCode(), MirInstructionOpCode::AND);

    // Ensure mask size matches original register bits: 1 byte -> mask 0xFF
    auto *maskOps = maskInstr->getOperands();
    EXPECT_EQ(maskOps->get<MirOperand>(1)->getInteger()->m_value, 0xFF);

    ec->endScope(ErrorAction::Discard);
}

// ─── Edge cases / Additional tests ─────────────────────────────────────────────

TEST_F(MirTypeLegalizerTests, ExpandMovWithImmediate)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    MirRegister dest = emitter->createVirtualRegister(8); // 64-bit on 32-bit machine
    MirOperand immOp(MirInteger{ 0x123456789ABCDEF0, 8 });
    emitter->emitMOV(MirOperand(dest), immOp);

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);
    EXPECT_EQ(block->getInstructions()->m_numElems, 2u); // should expand to two 32-bit MOVs

    auto it = block->getInstructions()->begin();
    MirInstruction *mov1 = *it;
    EXPECT_EQ(mov1->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(mov1->getOperands()->get<MirOperand>(1)->getInteger()->m_value, 0x9ABCDEF0);

    ++it;
    MirInstruction *mov2 = *it;
    EXPECT_EQ(mov2->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_EQ(mov2->getOperands()->get<MirOperand>(1)->getInteger()->m_value, 0x12345678);

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirTypeLegalizerTests, PromoteMovWithImmediate)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    MirRegister dest = emitter->createVirtualRegister(2); // 16-bit
    MirOperand immOp(MirInteger{ 0x1234, 2 });
    emitter->emitMOV(MirOperand(dest), immOp);

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);
    EXPECT_EQ(block->getInstructions()->m_numElems, 1u);

    auto it = block->getInstructions()->begin();
    MirInstruction *mov = *it;
    EXPECT_EQ(mov->getOpCode(), MirInstructionOpCode::MOV);

    auto *ops = mov->getOperands();
    EXPECT_EQ(ops->get<MirOperand>(0)->getRegister()->m_sizeInBytes, 4u); // promoted dest
    EXPECT_EQ(ops->get<MirOperand>(1)->getInteger()->m_sizeInBytes, 4u);  // promoted imm

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirTypeLegalizerTests, ExpandStoreWithImmediate)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    MirRegister ptrReg = emitter->createVirtualRegister(4); // Legal pointer size
    MirOperand immOp(MirInteger{ 0x1122334455667788, 8 });
    emitter->emitSTORE(MirOperand(ptrReg), immOp);

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);
    // STORE ptr, imm8 -> STORE ptr, imm4; ADD nextPtr, ptr, 4; STORE nextPtr, imm4
    EXPECT_EQ(block->getInstructions()->m_numElems, 3u);

    auto it = block->getInstructions()->begin();
    MirInstruction *st1 = *it;
    EXPECT_EQ(st1->getOpCode(), MirInstructionOpCode::STORE);
    EXPECT_EQ(st1->getOperands()->get<MirOperand>(1)->getInteger()->m_value, 0x55667788);

    ++it;
    MirInstruction *add = *it;
    EXPECT_EQ(add->getOpCode(), MirInstructionOpCode::ADD);

    ++it;
    MirInstruction *st2 = *it;
    EXPECT_EQ(st2->getOpCode(), MirInstructionOpCode::STORE);
    EXPECT_EQ(st2->getOperands()->get<MirOperand>(1)->getInteger()->m_value, 0x11223344);

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirTypeLegalizerTests, ExpandLoadWithOffset)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    MirRegister dest = emitter->createVirtualRegister(8);
    MirRegister ptrReg = emitter->createVirtualRegister(4);
    MirOperand offsetOp(MirInteger{ 16, 4 });

    // We can emit load directly or simulate it. We will emit instructions manually if emitter emitLOAD doesn't take 3
    // operands
    auto *loadOps = emitterCtx->getOperandPool()->createSlice<MirOperand>();
    emitterCtx->getOperandPool()->appendToSlice(loadOps, emitterCtx->getOperandPool()->create<MirOperand>(dest));
    emitterCtx->getOperandPool()->appendToSlice(loadOps, emitterCtx->getOperandPool()->create<MirOperand>(ptrReg));
    emitterCtx->getOperandPool()->appendToSlice(loadOps, emitterCtx->getOperandPool()->create<MirOperand>(offsetOp));
    MirInstruction *loadInstr =
            emitterCtx->getInstructionPool()->create<MirInstruction>(MirInstructionOpCode::LOAD, loadOps);
    emitterCtx->getInstructionPool()->appendToSlice(block->getInstructions(), loadInstr);

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);
    // LOAD dest8, ptr4, offset4 -> LOAD dest4_L, ptr4, offset4; ADD nextPtr, ptr4, 4; LOAD dest4_H, nextPtr, offset4
    EXPECT_EQ(block->getInstructions()->m_numElems, 3u);

    auto it = block->getInstructions()->begin();
    MirInstruction *ld1 = *it;
    EXPECT_EQ(ld1->getOpCode(), MirInstructionOpCode::LOAD);
    EXPECT_EQ(ld1->getOperands()->m_numElems, 3u); // should preserve offset

    ++it;
    MirInstruction *add = *it;
    EXPECT_EQ(add->getOpCode(), MirInstructionOpCode::ADD);
    EXPECT_EQ(add->getOperands()->get<MirOperand>(2)->getInteger()->m_value, 4);

    ++it;
    MirInstruction *ld2 = *it;
    EXPECT_EQ(ld2->getOpCode(), MirInstructionOpCode::LOAD);
    EXPECT_EQ(ld2->getOperands()->m_numElems, 3u); // should preserve offset

    ec->endScope(ErrorAction::Discard);
}

TEST_F(MirTypeLegalizerTests, ExpandSubProducesSubAndSbb)
{
    ec->beginScope();
    MirFunction *func = emitterCtx->createFunction(1);
    MirBlock *block = func->getEntryPoint();
    emitterCtx->bindToBlock(block);

    MirRegister dest = emitter->createVirtualRegister(8);
    MirRegister src = emitter->createVirtualRegister(8);
    emitter->emitSUB(MirOperand(dest), MirOperand(src));

    MirTypeLegalizerPass pass(legalizerCtx.get());
    bool changed = pass.run(func, nullptr);

    EXPECT_TRUE(changed);
    EXPECT_EQ(block->getInstructions()->m_numElems, 2u);

    auto it = block->getInstructions()->begin();
    MirInstruction *subInstr = *it;
    EXPECT_EQ(subInstr->getOpCode(), MirInstructionOpCode::SUB);

    ++it;
    MirInstruction *sbbInstr = *it;
    EXPECT_EQ(sbbInstr->getOpCode(), MirInstructionOpCode::SBB); // Borrow from first

    ec->endScope(ErrorAction::Discard);
}