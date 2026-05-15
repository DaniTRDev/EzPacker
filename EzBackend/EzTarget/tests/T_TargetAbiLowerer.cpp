/**
 * @file T_TargetAbiLowerer.cpp
 * @brief Unit tests for the TargetAbiLowererPass.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetAbiLowerer/TargetAbiLowererPass.h>
#include <filesystem>
#include <map>
#include <variant>

class DummyABIDesc : public ABIDesc
{
  public:
    DummyABIDesc()
    {
        setRegSizeInBits(64);
        // Default return in reg 0
        PhysicalRegLocation retLoc;
        retLoc.m_id = 0;
        retLoc.m_sizeInBytes = 8;
        ArgLocation argRetLoc;
        argRetLoc.setLoc(retLoc);
        setReturnValueLoc(argRetLoc);
    }

    ArgLocation getArgLoc(size_t id) const override
    {
        if (m_argLocs.count(id))
        {
            return m_argLocs.at(id);
        }
        // Return an uninitialized ArgLocation to signify no specific ABI rule.
        // The pass should ideally handle this gracefully.
        return ArgLocation();
    }

    void setArgLoc(size_t id, const ArgLocation &loc) { m_argLocs[id] = loc; }

  private:
    std::map<size_t, ArgLocation> m_argLocs;
};

class TargetAbiLowererTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    std::shared_ptr<MirTypes> m_types;
    MirEmitter *emitter;

    DummyABIDesc abi;
    TargetDesc *targetDesc;
    TargetAbiLowererContext *lowererCtx;
    TargetAbiLowererPass *pass;
    MirPassManager *passManager;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());

        m_types = std::make_shared<MirTypes>();
        m_types->initialize(emitterCtx.get());

        targetDesc = new TargetDesc(&abi, "DummyTarget");
        lowererCtx = new TargetAbiLowererContext(emitter, targetDesc);

        pass = new TargetAbiLowererPass(lowererCtx);
        passManager = new MirPassManager();

        ec->beginScope();
    }

    bool runAbiPass()
    {
        auto funcList = emitterCtx->getFunctionList();
        bool modified = false;

        for (auto it = funcList->begin(); it != funcList->end(); ++it)
        {
            modified |= pass->run(funcList, it, passManager);
        }

        return modified;
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete passManager;
        delete pass;
        delete lowererCtx;
        delete targetDesc;
        delete emitter;
    }
};

TEST_F(TargetAbiLowererTests, LowersFunctionParameterInRegister)
{
    // Setup ABI: first argument in physical register 5
    PhysicalRegLocation loc;
    loc.m_id = 5;
    loc.m_sizeInBytes = 8;
    ArgLocation argLoc;
    argLoc.setLoc(loc);
    abi.setArgLoc(0, argLoc);

    MirType *int64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!int64Type)
    {
        int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(int64Type, nullptr, "testFunc");
    MirRegister *param = emitter->createVirtualRegister(int64Type);
    func->appendParameter(param, "a1");

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    EXPECT_TRUE(runAbiPass());

    // Verify that a MOV instruction was inserted
    MirBlock *entryBlock = func->getEntryPoint();
    ASSERT_EQ(entryBlock->getInstructions()->m_numElems, 1);
    MirInstruction *movInstr = *entryBlock->getInstructions()->begin();
    EXPECT_EQ(movInstr->getOpCode(), MirInstructionOpCode::MOV);

    // Verify operands of MOV
    auto operands = movInstr->getOperands();
    ASSERT_EQ(operands->m_numElems, 2);
    auto opIt = operands->begin();
    MirOperand *dest = *opIt;
    ++opIt;
    MirOperand *src = *opIt;

    EXPECT_EQ(dest, param); // Destination should be the virtual register parameter

    ASSERT_TRUE(src->isOfType<MirRegister>());
    MirRegister *srcReg = src->get<MirRegister>();
    EXPECT_TRUE(!srcReg->isVirtual());
    EXPECT_EQ(srcReg->getRegId(), 5);
}

TEST_F(TargetAbiLowererTests, LowersFunctionParameterOnStack)
{
    // Setup ABI: first argument on stack at offset 16
    StackLocation loc;
    loc.m_offset = 16;
    ArgLocation argLoc;
    argLoc.setLoc(loc);
    abi.setArgLoc(0, argLoc);

    MirType *int64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!int64Type)
    {
        int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(int64Type, nullptr, "testFunc");
    MirRegister *param = emitter->createVirtualRegister(int64Type);
    func->appendParameter(param, "a1");

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    EXPECT_TRUE(runAbiPass());

    // Verify that a LOAD instruction was inserted
    MirBlock *entryBlock = func->getEntryPoint();
    ASSERT_EQ(entryBlock->getInstructions()->m_numElems, 1);
    MirInstruction *loadInstr = *entryBlock->getInstructions()->begin();
    EXPECT_EQ(loadInstr->getOpCode(), MirInstructionOpCode::LOAD);

    // Verify operands of LOAD
    auto operands = loadInstr->getOperands();
    ASSERT_EQ(operands->m_numElems, 2);
    auto opIt = operands->begin();
    MirOperand *dest = *opIt;
    ++opIt;
    MirOperand *src = *opIt;

    EXPECT_EQ(dest, param); // Destination should be the virtual register parameter

    ASSERT_TRUE(src->isOfType<MirMemory>());
    MirMemory *memOp = src->get<MirMemory>();
    MirOperand *base = memOp->getBase();
    ASSERT_TRUE(base->isOfType<MirFrameIndex>());
    MirFrameIndex *frameIndex = base->get<MirFrameIndex>();

    // Check that a stack frame object was created for the parameter
    MirFunctionStackFrame *stackFrame = func->getStackFrame();
    const StackFrameObject *sfo = stackFrame->getObjectFromId(frameIndex->getFrameId());
    EXPECT_EQ(sfo->m_offset, 16);
    EXPECT_TRUE(sfo->m_source == StackFrameObjectSource::Parameter);
}

TEST_F(TargetAbiLowererTests, LowersCallSiteArgumentInRegister)
{
    // Setup ABI: first argument in physical register 7
    PhysicalRegLocation loc;
    loc.m_id = 7;
    loc.m_sizeInBytes = 8;
    ArgLocation argLoc;
    argLoc.setLoc(loc);
    abi.setArgLoc(0, argLoc);

    MirType *int64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!int64Type)
    {
        int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *callee = emitterCtx->createFunction(int64Type, nullptr, "callee");
    MirRegister *param = emitter->createVirtualRegister(m_types->getInt64Type());
    callee->appendParameter(param, "a1");

    MirFunction *func = emitterCtx->createFunction(int64Type, nullptr, "testFunc");
    MirRegister *arg = emitter->createVirtualRegister(int64Type);
    emitter->emit(MirInstructionOpCode::CALL, { emitter->createReference(callee->getEntryPoint()), arg });

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    EXPECT_TRUE(runAbiPass());

    // Verify that a MOV instruction was inserted before the CALL
    ASSERT_EQ(func->getEntryPoint()->getInstructions()->m_numElems, 2);
    auto instrIt = func->getEntryPoint()->getInstructions()->begin();
    MirInstruction *movInstr = *instrIt;
    EXPECT_EQ(movInstr->getOpCode(), MirInstructionOpCode::MOV);

    ++instrIt;
    MirInstruction *callInstr = *instrIt;
    EXPECT_EQ(callInstr->getOpCode(), MirInstructionOpCode::CALL);

    // Verify MOV operands
    auto movOperands = movInstr->getOperands();
    ASSERT_EQ(movOperands->m_numElems, 2);
    auto movOpIt = movOperands->begin();
    MirOperand *dest = *movOpIt;
    ++movOpIt;
    MirOperand *src = *movOpIt;

    EXPECT_EQ(src, arg); // Source of MOV is the virtual register argument
    ASSERT_TRUE(dest->isOfType<MirRegister>());
    MirRegister *destReg = dest->get<MirRegister>();
    EXPECT_TRUE(!destReg->isVirtual());
    EXPECT_EQ(destReg->getRegId(), 7);

    // Verify that the CALL argument was replaced with the physical register
    auto callOperands = callInstr->getOperands();
    ASSERT_EQ(callOperands->m_numElems, 2);
    auto callOpIt = callOperands->begin();
    ++callOpIt; // skip callee
    MirOperand *callArg = *callOpIt;
    EXPECT_EQ(callArg, dest); // Call argument should be the physical register
}

TEST_F(TargetAbiLowererTests, LowersCallSiteArgumentOnStack)
{
    // Setup ABI: first argument on stack at offset 24 from stack pointer
    StackLocation loc;
    loc.m_offset = 24;
    ArgLocation argLoc;
    argLoc.setLoc(loc);
    abi.setArgLoc(0, argLoc);
    abi.setStackReg(1); // Let's say stack pointer is physical reg 1

    MirType *int64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!int64Type)
    {
        int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }
    MirType *ptrType = emitterCtx->getIntegerTypeBySize(8); // Assuming 64-bit pointers
    if (!ptrType)
    {
        ptrType = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *callee = emitterCtx->createFunction(int64Type, nullptr, "callee");
    MirRegister *arg = emitter->createVirtualRegister(int64Type);
    callee->appendParameter(arg, "a1");

    MirFunction *func = emitterCtx->createFunction(int64Type, nullptr, "testFunc");
    emitter->emit(MirInstructionOpCode::CALL, { emitter->createReference(callee->getEntryPoint()), arg });

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    EXPECT_TRUE(runAbiPass());

    // Verify that a STORE instruction was inserted before the CALL
    ASSERT_EQ(func->getEntryPoint()->getInstructions()->m_numElems, 2);
    auto instrIt = func->getEntryPoint()->getInstructions()->begin();
    MirInstruction *storeInstr = *instrIt;
    EXPECT_EQ(storeInstr->getOpCode(), MirInstructionOpCode::STORE);

    ++instrIt;
    MirInstruction *callInstr = *instrIt;
    EXPECT_EQ(callInstr->getOpCode(), MirInstructionOpCode::CALL);

    // Verify STORE operands
    auto storeOperands = storeInstr->getOperands();
    ASSERT_EQ(storeOperands->m_numElems, 2);
    auto storeOpIt = storeOperands->begin();
    MirOperand *dest = *storeOpIt;
    ++storeOpIt;
    MirOperand *src = *storeOpIt;

    EXPECT_EQ(src, arg); // Source of STORE is the virtual register argument
    ASSERT_TRUE(dest->isOfType<MirMemory>());
    MirMemory *memOp = dest->get<MirMemory>();
    MirOperand *base = memOp->getBase();
    MirOperand *offset = memOp->getDisplacement();

    ASSERT_TRUE(base->isOfType<MirRegister>());
    MirRegister *baseReg = base->get<MirRegister>();
    EXPECT_TRUE(!baseReg->isVirtual());
    EXPECT_EQ(baseReg->getRegId(), 1); // Stack pointer

    ASSERT_TRUE(offset->isOfType<MirInteger>());
    MirInteger *offsetVal = offset->get<MirInteger>();
    EXPECT_EQ(offsetVal->getValue(), 24);

    // Verify that the argument was removed from the CALL instruction
    auto callOperands = callInstr->getOperands();
    ASSERT_EQ(callOperands->m_numElems, 1);

    auto calleeOp = *callOperands->begin();
    ASSERT_TRUE(calleeOp->isOfType<MirReference>());
}

TEST_F(TargetAbiLowererTests, LowersReturnSite)
{
    // ABI default sets return value to physical register 0
    MirType *int64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!int64Type)
    {
        int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(int64Type,
                                                   emitterCtx->getOperandPool()->createLinkedList<MirOperand *>(),
                                                   "testFunc");
    MirRegister *retVal = emitter->createVirtualRegister(int64Type);
    emitter->emit(MirInstructionOpCode::RET, { retVal });

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    EXPECT_TRUE(runAbiPass());

    // Verify that a MOV instruction was inserted before the RET
    ASSERT_EQ(func->getEntryPoint()->getInstructions()->m_numElems, 2);
    auto instrIt = func->getEntryPoint()->getInstructions()->begin();
    MirInstruction *movInstr = *instrIt;
    EXPECT_EQ(movInstr->getOpCode(), MirInstructionOpCode::MOV);

    ++instrIt;
    MirInstruction *retInstr = *instrIt;
    EXPECT_EQ(retInstr->getOpCode(), MirInstructionOpCode::RET);

    // Verify MOV operands
    auto movOperands = movInstr->getOperands();
    ASSERT_EQ(movOperands->m_numElems, 2);
    auto movOpIt = movOperands->begin();
    MirOperand *dest = *movOpIt;
    ++movOpIt;
    MirOperand *src = *movOpIt;

    EXPECT_EQ(src, retVal); // Source of MOV is the virtual return value
    ASSERT_TRUE(dest->isOfType<MirRegister>());
    MirRegister *destReg = dest->get<MirRegister>();
    EXPECT_TRUE(!destReg->isVirtual());
    EXPECT_EQ(destReg->getRegId(), 0); // Default return register

    // Verify that the RET operand was replaced with the physical register
    auto retOperands = retInstr->getOperands();
    ASSERT_EQ(retOperands->m_numElems, 1);
    MirOperand *retOp = *retOperands->begin();
    EXPECT_EQ(retOp, dest); // RET operand should be the physical register
}

TEST_F(TargetAbiLowererTests, VoidReturnIsNotModified)
{
    MirType *voidType = m_types->getVoidType();
    if (!voidType)
    {
        voidType = emitterCtx->createType(MirTypeKind::Void, 0, nullptr, "void");
    }

    MirFunction *func = emitterCtx->createFunction(voidType,
                                                   emitterCtx->getOperandPool()->createLinkedList<MirOperand *>(),
                                                   "testFunc");
    MirBlock *block = emitterCtx->createBlock();
    emitterCtx->bindToBlock(block);
    emitterCtx->getBlockPool()->appendToListBack(func->getBlocks(), block);

    emitter->emit(MirInstructionOpCode::RET, {}); // Void return has no operands

    auto funcList = emitterCtx->getFunctionList();
    emitterCtx->getFunctionPool()->appendToListBack(funcList, func);
    auto it = funcList->begin();

    EXPECT_FALSE(runAbiPass());

    // Verify that no instructions were added
    ASSERT_EQ(block->getInstructions()->m_numElems, 1);
    MirInstruction *retInstr = *block->getInstructions()->begin();
    EXPECT_EQ(retInstr->getOpCode(), MirInstructionOpCode::RET);
}
