#include "EzTripleTestSuite.h"

class TestFrameLowererPass : public MirTripleTestSuiteAsGtest
{
  public:
};

// =========================================================================
// 1. EMPTY / LEAF FUNCTION (NO LOCAL OBJECTS, NO CALLEE-SAVED REGS)
// =========================================================================
TEST_F(TestFrameLowererPass, LowerEmptyLeafFunction)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    // Build entry block containing a RET instruction
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    iBuilder.RET();

    // Execute Frame Lowerer Pass
    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    // Verify Prologue, Epilogue, and Stack Reference Resolution via Pass Verifier
    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);

    verifier.verifyPrologue(func);
    verifier.verifyEpilogue(func);
    verifier.verifyStackReferencesLowered(func);
}

// =========================================================================
// 2. FUNCTION WITH LOCAL STACK VARIABLES & SPILLS
// =========================================================================
TEST_F(TestFrameLowererPass, LowerFunctionWithLocalVariablesAndSpills)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirFunctionStackFrame *frame = func->getStackFrame();
    EXPECT_NE(frame, nullptr);

    // Allocate stack objects for local variables and spill slots
    StackFrameObject *var0 = frame->createStaticStackObj(t->i32());
    StackFrameObject *var1 = frame->createStaticStackObj(t->i64());
    StackFrameObject *spill0 = frame->createStackSpill(t->f64());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    // Create virtual registers and stack references
    MirRegister *vReg1 = oBuilder.buildVReg(t->i32(), "val32");
    MirRegister *vReg2 = oBuilder.buildVReg(t->f64(), "valFp");

    MirOperand *stackRefVar0 = oBuilder.buildRef(var0);
    MirOperand *stackRefSpill0 = oBuilder.buildRef(spill0);

    // Emit abstract instructions operating on abstract StackFrameObjects
    // STORE [var0], vReg1
    iBuilder.STORE(stackRefVar0, vReg1);
    // STORE [spill0], vReg2
    iBuilder.STORE(stackRefSpill0, vReg2);
    // RET
    iBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);

    // Frame allocation size must account for i32 (4), i64 (8), and f64 (8) + alignment
    EXPECT_GT(func->getAnalysisData()->m_totalFrameSize, 0u);

    verifier.verifyPrologue(func);
    verifier.verifyEpilogue(func);
    // Ensure all MirReference::StackFrameObject operands were converted into concrete MirMemory [FP/SP + offset]
    verifier.verifyStackReferencesLowered(func);
}

// =========================================================================
// 3. CALLEE-SAVED REGISTER PRESERVATION & RESTORATION
// =========================================================================
TEST_F(TestFrameLowererPass, LowerCalleeSavedRegisters)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    // Register allocations used callee-saved physical registers
    CallingConvDesc *cc = func->getCallingConv();
    EXPECT_NE(cc, nullptr);

    // Retrieve all callee-saved registers for this target/ABI
    const auto &calleeSavedList = cc->getAllCalleeSavedRegs();
    ASSERT_FALSE(calleeSavedList.empty());

    RegisterRef reg1 = calleeSavedList.front();
    func->addCalleeSavedRegUse(reg1);

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    iBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);

    // Callee-saved area size should cover the register
    EXPECT_GT(func->getAnalysisData()->m_calleeSavedAreaSize, 0u);

    // Verifiers ensure:
    // - Prologue pushes reg1
    // - Epilogue pops reg1 before RET
    verifier.verifyPrologue(func);
    verifier.verifyEpilogue(func);
    verifier.verifyStackReferencesLowered(func);
}

// =========================================================================
// 4. MULTIPLE EXIT BLOCKS (MULTIPLE RETURN EPILOGUES)
// =========================================================================
TEST_F(TestFrameLowererPass, LowerMultipleReturnBlocks)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirBlock *entryBlock = func->getEntryPoint();
    MirBlock *thenBlock = funcBuilder.blockBuilder().build(nullptr, "then_block");
    MirBlock *elseBlock = funcBuilder.blockBuilder().build(nullptr, "else_block");

    MirInstructionBuilder entryBuilder(getBuilderCtx(),
                                       entryBlock,
                                       InsertionType::InsertAfter,
                                       entryBlock->getInstructions().begin());
    MirInstructionBuilder thenBuilder(getBuilderCtx(),
                                      thenBlock,
                                      InsertionType::InsertAfter,
                                      thenBlock->getInstructions().begin());
    MirInstructionBuilder elseBuilder(getBuilderCtx(),
                                      elseBlock,
                                      InsertionType::InsertAfter,
                                      elseBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    // Allocate local object to force stack layout generation
    func->getStackFrame()->createStaticStackObj(t->i64());

    // Branch to then / else
    entryBuilder.JE(oBuilder.buildRef(thenBlock));
    entryBuilder.JMP(oBuilder.buildRef(elseBlock));

    // Both blocks terminate with return instructions
    thenBuilder.RET();
    elseBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);

    // Prologue must only be injected once at entryBlock
    verifier.verifyPrologue(func);
    // Epilogue must be injected before RET in BOTH thenBlock and elseBlock
    verifier.verifyEpilogue(func);
    verifier.verifyStackReferencesLowered(func);
}

// =========================================================================
// 5. COMBINED LARGE FRAME ALLOCATION & CALLEE-SAVED REGS
// =========================================================================
TEST_F(TestFrameLowererPass, LowerComplexStackLayout)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    CallingConvDesc *cc = func->getCallingConv();
    const auto &calleeSavedList = cc->getAllCalleeSavedRegs();
    if (!calleeSavedList.empty())
    {
        func->addCalleeSavedRegUse(calleeSavedList.front());
    }

    MirFunctionStackFrame *frame = func->getStackFrame();
    // Create multiple stack objects to simulate a large frame payload
    for (size_t i = 0; i < 16; ++i)
    {
        frame->createStaticStackObj(t->i64());
    }

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirOperandBuilder oBuilder(getBuilderCtx());
    MirRegister *regVal = oBuilder.buildVReg(t->i64(), "temp");

    // Access first and last stack objects
    MirOperand *firstObjRef = oBuilder.buildRef(frame->getObjects().front());
    MirOperand *lastObjRef = oBuilder.buildRef(frame->getObjects().back());

    iBuilder.LOAD(regVal, firstObjRef);
    iBuilder.STORE(lastObjRef, regVal);
    iBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);

    verifier.verifyPrologue(func);
    verifier.verifyEpilogue(func);
    verifier.verifyStackReferencesLowered(func);
}

// =========================================================================
// 6. DYNAMIC ALLOCATION (DALLOC) LOWERING TEST
// =========================================================================
TEST_F(TestFrameLowererPass, LowerDynamicStackAllocation)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType(), "LowerDynamicStackAllocation");

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    // Allocate virtual registers for dynamic size & pointer destination
    MirRegister *dynPtr = oBuilder.buildVReg(t->getPtr(t->i8()), "dynPtr");
    MirRegister *runtimeSize = oBuilder.buildVReg(t->i64(), "runtimeSize");

    // Initialize runtime size variable (e.g. MOV %runtimeSize, 128)
    iBuilder.MOV(runtimeSize, oBuilder.buildInt(t->i64(), FlexInt(128, 64)));

    // Emit DALLOC instruction: %dynPtr = DALLOC %runtimeSize
    iBuilder.DALLOC(dynPtr, runtimeSize);

    // Emit a dummy RET instruction to close the block
    iBuilder.RET();

    // 1. Run Register Allocator Pass (Flags func->setHasDynamicAllocs(true) and reserves FP)
    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getRegisterAllocator(), getTargetDesc());

    // 2. Run Frame Lowerer Pass (Lowers DALLOC, calculates layout, inserts Prologue/Epilogue)
    MirFrameLowererPass *framePass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    // 3. Verify Frame Lowerer execution using the updated verifiers
    FrameLowererPassVerifier verifier(getBuilderCtx(), framePass);
    verifier.verifyDAllocLowered(func).verifyPrologue(func).verifyEpilogue(func).verifyStackReferencesLowered(func);

    // 4. Assert structural invariants
    EXPECT_TRUE(func->getAnalysisData()->m_hasDynamicAllocs);
    EXPECT_EQ(entryBlock->getInstructions().back()->getOpCode(), MirInstructionOpCode::RET);
}