/**
 * @file T_TargetStackFrameLowerer.cpp
 * @brief Unit tests for the StackFrameLowererPass pass.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetStackFrameLowerer/StackFrameLowerer.h>
#include <TargetStackFrameLowerer/StackFrameLowererContext.h>
#include <filesystem>

class DummyABIDesc : public ABIDesc
{
  public:
    DummyABIDesc()
    {
        setRegSizeInBits(64);
        setStackReg(1);
        setStackFrame(2);
        setStackLayout(StackLayout(8, 0));
    }

    ArgLocation getArgLoc(size_t id, MirType *type) const override
    {
        return ArgLocation();
    }
    
    size_t getAbiAlignment(MirType *type) const
    {
        if (!type)
            return 1;
        
        // Handle Basic Types (Integers, Floats)
        size_t size = type->getTotalSizeInBytes();

        // Standard rule: basic types align to their own size, capped by the target max.
        // E.g., size 4 aligns to 4. Size 8 aligns to 8 (or 4 on 32-bit systems).
        size_t align = size;

        // Ensure it's a power of 2 (rounds up sizes like 3 to 4)
        align = std::bit_ceil(align); // C++20 feature, or write a quick power-of-2 helper
        return align;
    }

    const char *getName() const override { return "DummyAbi"; }
    
    /**
     * Returns the preferred alignment for the given type.
     * Used for global variables to optimize CPU cache line fetching.
     */
    size_t getPreferredAlignment(MirType *type) const { return getAbiAlignment(type); }
};

class TargetStackFrameLowererTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;

    DummyABIDesc abi;
    TargetDesc *targetDesc;
    StackFrameLowererContext *lowererCtx;
    MirPassManager *passManager;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());

        targetDesc = new TargetDesc(&abi, TargetEndianness::LittleEndian, "DummyTarget");
        lowererCtx = new StackFrameLowererContext(emitter, targetDesc);
        
        passManager = new MirPassManager();
        passManager->addPass<StackFrameLowererPass>(lowererCtx);
        
        ec->beginScope();
    }

    bool runPass()
    {
        auto funcList = emitterCtx->getFunctionList();
        bool modified = false;

        for (auto it = funcList->begin(); it != funcList->end(); ++it)
        {
            modified |= passManager->run(funcList, it, passManager);
        }

        return modified;
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete passManager;
        delete lowererCtx;
        delete targetDesc;
        delete emitter;
    }
};

TEST_F(TargetStackFrameLowererTests, CalculatesOffsetsCorrectly)
{
    MirType *int64Type = emitterCtx->getTypes()->getInt64Type();
    if (!int64Type) int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");

    MirFunction *func = emitterCtx->createFunction(int64Type, nullptr, "testFunc");
    MirFunctionStackFrame *stackFrame = func->getStackFrame();

    // Create objects
    StackFrameObject *fixedObj = stackFrame->createParam(8, 8, 8); // size 8, align 8, offset 8
    StackFrameObject *localObj1 = stackFrame->createLocalObject(4, 4); // size 4, align 4
    StackFrameObject *localObj2 = stackFrame->createLocalObject(8, 8); // size 8, align 8

    // Dummy block with RET to satisfy lowerer
    MirBlock *entryBlock = func->getEntryPoint();
    emitterCtx->setInsertPoint(entryBlock);
    emitter->emit(MirInstructionOpCode::RET, {});

    EXPECT_TRUE(runPass());

    EXPECT_EQ(fixedObj->m_offset, 8);
    // Unallocated locals start at offset 0. Since 0..8 is free, localObj1 can go at 0.
    EXPECT_EQ(localObj1->m_offset, 0);

    // After localObj1, currentOffset is 4. localObj2 size 8.
    // Alignment to 8 means it tries offset 8.
    // [8, 16] collides with fixedObj [8, 16].
    // It steps forward + 1, aligned to 8 -> 16.
    // [16, 24] doesn't collide.
    EXPECT_EQ(localObj2->m_offset, 16);
}

TEST_F(TargetStackFrameLowererTests, InsertsPrologueAndEpilogue)
{
    MirType *int64Type = emitterCtx->getTypes()->getInt64Type();
    if (!int64Type) int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");

    MirFunction *func = emitterCtx->createFunction(int64Type, nullptr, "testFunc");
    MirFunctionStackFrame *stackFrame = func->getStackFrame();

    // Size 16 local variable to get non-zero stackFrameEndOffset
    stackFrame->createLocalObject(16, 8);

    MirBlock *entryBlock = func->getEntryPoint();
    emitterCtx->setInsertPoint(entryBlock);
    
    MirRegister *vreg = emitter->createVirtualRegister(int64Type);
    emitter->emit(MirInstructionOpCode::MOV, { vreg, vreg }); // Dummy
    emitter->emit(MirInstructionOpCode::RET, {});

    EXPECT_TRUE(runPass());

    auto instrList = entryBlock->getInstructions();
    auto it = instrList->begin();

    // Prologue
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::SUB); // SUB SP, fpSize
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::STORE); // STORE [SP], FP
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV); // MOV FP, SP
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::SUB); // SUB SP, frameSize
    
    // Check frameSize == 16
    auto operands = (*it)->getOperands();
    auto opIt = operands->begin(); ++opIt; // Second operand is immediate
    ASSERT_TRUE((*opIt)->isOfType<MirInteger>());
    EXPECT_EQ((*opIt)->get<MirInteger>()->getValue(), 16);
    ++it;

    // Body
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV); // Dummy MOV
    ++it;

    // Epilogue
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV); // MOV SP, FP
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::LOAD); // LOAD FP, [SP]
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::ADD); // ADD SP, fpSize
    ++it;

    // Return
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::RET);
}

TEST_F(TargetStackFrameLowererTests, EpilogueInMultipleBlocks)
{
    MirType *int64Type = emitterCtx->getTypes()->getInt64Type();
    if (!int64Type) int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");

    MirFunction *func = emitterCtx->createFunction(int64Type, nullptr, "testFunc");
    MirBlock *entryBlock = func->getEntryPoint();
    emitter->emit(MirInstructionOpCode::RET, {});

    MirBlock *block2 = emitterCtx->createBlock();
    emitterCtx->setInsertPoint(block2);
    emitter->emit(MirInstructionOpCode::RET, {});

    EXPECT_TRUE(runPass());

    // Epilogue validation in entry block
    auto it1 = entryBlock->getInstructions()->rbegin();
    ++it1; // Skip RET
    ++it1; // Skip ADD SP, fpSize
    ++it1; // Skip LOAD FP, [SP]
    EXPECT_EQ((*it1)->getOpCode(), MirInstructionOpCode::MOV);

    // Epilogue validation in block 2
    auto it2 = block2->getInstructions()->rbegin();
    ++it2; // Skip RET
    ++it2; // Skip ADD SP, fpSize
    ++it2; // Skip LOAD FP, [SP]
    EXPECT_EQ((*it2)->getOpCode(), MirInstructionOpCode::MOV);
}

TEST_F(TargetStackFrameLowererTests, EmptyFunctionWithoutLocalVars)
{
    MirType *int64Type = emitterCtx->getTypes()->getInt64Type();
    if (!int64Type) int64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");

    MirFunction *func = emitterCtx->createFunction(int64Type, nullptr, "testFunc");
    MirBlock *entryBlock = func->getEntryPoint();
    emitter->emit(MirInstructionOpCode::RET, {});

    EXPECT_TRUE(runPass());

    auto it = entryBlock->getInstructions()->begin();
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::SUB); // SUB SP, fpSize
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::STORE); // STORE [SP], FP
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV); // MOV FP, SP
    ++it;
    
    // The next one should be the Epilogue's MOV SP, FP, no "SUB SP, frameSize"
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV);
}
