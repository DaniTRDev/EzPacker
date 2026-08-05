/**
 * @file TestFrameLowererPass.cpp
 * @brief Unit tests for MirFrameLowererPass verifying frame layout generation, prologue/epilogue
 * insertion, callee-saved register handling, and stack object reference resolution.
 */

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
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    // Verify Prologue, Epilogue, and Stack Reference Resolution via Pass Verifier
    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    const FrameLayout &layout = pass->getResult().m_layouts.at(func);

    verifier.verifyPrologue(func, layout);
    verifier.verifyEpilogue(func, layout);
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
    StackFrameObject *var0 = frame->createLocalStackObj(t->i32());
    StackFrameObject *var1 = frame->createLocalStackObj(t->i64());
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

    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    const FrameLayout &layout = pass->getResult().m_layouts.at(func);

    // Frame allocation size must account for i32 (4), i64 (8), and f64 (8) + alignment
    EXPECT_GT(layout.totalFrameSize, 0u);

    verifier.verifyPrologue(func, layout);
    verifier.verifyEpilogue(func, layout);
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

    // Add physical callee-saved registers to the function
    const auto &calleeSavedList = cc->getCalleeSavedRegs(RegisterRefClass::GPR);

    RegisterRef reg1 = calleeSavedList[0];
    func->addCalleeSavedRegUse(reg1);

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    iBuilder.RET();

    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    const FrameLayout &layout = pass->getResult().m_layouts.at(func);

    // Callee-saved area size should cover the register.
    EXPECT_GT(layout.calleeSavedAreaSize, 0u);

    // Verifiers ensure:
    // - Prologue pushes reg1
    // - Epilogue pops reg1 before RET
    verifier.verifyPrologue(func, layout);
    verifier.verifyEpilogue(func, layout);
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
    func->getStackFrame()->createLocalStackObj(t->i64());

    // Branch to then / else
    MirRegister *condReg = oBuilder.buildVReg(t->i1(), "cond");
    entryBuilder.JE(oBuilder.buildRef(thenBlock));
    entryBuilder.JMP(oBuilder.buildRef(elseBlock));

    // Both blocks terminate with return instructions
    thenBuilder.RET();
    elseBuilder.RET();

    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    const FrameLayout &layout = pass->getResult().m_layouts.at(func);

    // Prologue must only be injected once at entryBlock
    verifier.verifyPrologue(func, layout);
    // Epilogue must be injected before RET in BOTH thenBlock and elseBlock
    verifier.verifyEpilogue(func, layout);
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
    if (!cc->getCalleeSavedRegs(RegisterRefClass::GPR).empty())
    {
        func->addCalleeSavedRegUse(cc->getCalleeSavedRegs(RegisterRefClass::GPR).front());
    }

    MirFunctionStackFrame *frame = func->getStackFrame();
    // Create multiple stack objects to simulate a large frame payload
    for (size_t i = 0; i < 16; ++i)
    {
        frame->createLocalStackObj(t->i64());
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

    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    const FrameLayout &layout = pass->getResult().m_layouts.at(func);

    verifier.verifyPrologue(func, layout);
    verifier.verifyEpilogue(func, layout);
    verifier.verifyStackReferencesLowered(func);
}