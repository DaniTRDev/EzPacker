#include "EzTripleTestSuite.h"
#include "AbiLowerer/MirAbiLowerer.h"
#include "AbiLowerer/MirAbiLowererPass.h"
#include "Function/CallLoweringState.h"
#include "Function/MirFunctionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Operand/MirOperandBuilder.h"
#include "x86_64CallingConvDesc.h"

class MirAbiLowererTest : public EzTripleTestSuite
{
};

TEST_F(MirAbiLowererTest, TestReturnLowering)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("ret_lower", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *retVal = ob.buildVReg(typeTable->i32(), "val");
    ib.RET(retVal);

    // Legalize return first -> emits PUSH_RET + tokenized RET
    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    LegalizeActions::LegalizeReturn(legCtx);

    // Run ABI Lowerer Pass
    MirAbiLowererPass pass(ctx);
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
    EXPECT_TRUE(result.m_modifiedMir);

    // Verify lowered sequence contains MOV into return physical register (RAX) and RET
    bool foundMovToReg = false;
    for (MirInstruction *inst : block->getInstructions())
    {
        if (inst->getOpCodeName() == std::string("MOV"))
        {
            foundMovToReg = true;
        }
    }
    EXPECT_TRUE(foundMovToReg);
}

TEST_F(MirAbiLowererTest, TestCallAndArgLowering)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("call_lower", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dest = ob.buildVReg(typeTable->i32(), "res");
    MirRegister *arg0 = ob.buildVReg(typeTable->i32(), "x");
    MirRegister *arg1 = ob.buildVReg(typeTable->i32(), "y");
    MirReference *callee = ob.buildRef(func);

    ib.CALL(dest, callee, arg0, arg1);

    // Legalize call -> emits PUSH_ARG, PUSH_ARG, CALL, POP_RET
    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    LegalizeActions::LegalizeCall(legCtx);

    // Run ABI Lowerer Pass
    MirAbiLowererPass pass(ctx);
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
    EXPECT_TRUE(result.m_modifiedMir);

    // Verify CALL instructions lowered: PUSH_ARG / POP_RET removed and MOV instructions emitted
    for (MirInstruction *inst : block->getInstructions())
    {
        EXPECT_NE(inst->getOpCodeName(), std::string("PUSH_ARG"));
        EXPECT_NE(inst->getOpCodeName(), std::string("POP_RET"));
    }
}

TEST_F(MirAbiLowererTest, TestParameterLowering)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("param_lower", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirOperandBuilder ob(ctx);
    MirRegister *p0 = ob.buildVReg(typeTable->i32(), "p0");
    MirRegister *p1 = ob.buildVReg(typeTable->i32(), "p1");
    MirFunctionBuilder(ctx).addParam(func, p0).addParam(func, p1);

    // Legalize signature -> emits POP_ARG, POP_ARG, END_ARG
    MirFunctionSignatureLegalizerPass sigPass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);
    sigPass.run(funcList.begin(), nullptr);

    // Run ABI lowerer
    MirAbiLowererPass pass(ctx);
    auto result = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);

    // Verify POP_ARG replaced by MOV from argument registers (RDI, RSI)
    for (MirInstruction *inst : block->getInstructions())
    {
        EXPECT_NE(inst->getOpCodeName(), std::string("POP_ARG"));
    }
}

TEST_F(MirAbiLowererTest, TestSretLowering)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("sret_lower", typeTable->i256());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *retVal = ob.buildVReg(typeTable->i256(), "big_val");
    ib.RET(retVal);

    // Signature legalization -> prepends SRET pointer parameter
    MirFunctionSignatureLegalizerPass sigPass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);
    sigPass.run(funcList.begin(), nullptr);

    // Return legalization
    auto it = --block->getInstructions().end();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    LegalizeActions::LegalizeReturn(legCtx);

    // ABI Lowering
    MirAbiLowererPass pass(ctx);
    auto result = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
}

TEST_F(MirAbiLowererTest, TestSysVAMD64CallingConvention)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *gprClass = getTargetDesc()->getGprClass();
    auto *func = createTestFunction("test_sysv_func", typeTable->i64());

    SysV_AMD64CallingConvDesc cc(ctx, gprClass);
    EXPECT_STREQ(cc.getName(), "SysV_AMD64");
    EXPECT_EQ(cc.getStackAlignment(), 16);
    EXPECT_EQ(cc.getShadowSpaceSize(), 0);
    EXPECT_EQ(cc.getRedZoneSize(), 128);
    EXPECT_FALSE(cc.isCalleeCleanup());
    EXPECT_TRUE(cc.doesStackGrowsDownwards());
    EXPECT_TRUE(cc.consumesSretSlot());

    CallLoweringState callState(&cc, ctx, func);

    // 8 arguments of i64:
    // First 6 must be passed in registers: rdi, rsi, rdx, rcx, r8, r9
    for (size_t i = 0; i < 6; ++i)
    {
        auto loc = cc.getArgLoc(typeTable->i64(), &callState);
        EXPECT_EQ(loc.getType(), ArgLocationType::Register);
    }
    EXPECT_EQ(callState.getBankCursor("integer"), 6);

    // Remaining 2 arguments must be placed on the stack
    auto stackLoc1 = cc.getArgLoc(typeTable->i64(), &callState);
    EXPECT_EQ(stackLoc1.getType(), ArgLocationType::Stack);
    EXPECT_EQ(stackLoc1.getStack().m_sizeBytes, 8);

    auto stackLoc2 = cc.getArgLoc(typeTable->i64(), &callState);
    EXPECT_EQ(stackLoc2.getType(), ArgLocationType::Stack);
    EXPECT_EQ(stackLoc2.getStack().m_sizeBytes, 8);

    // Verify return location in RAX
    auto retLoc = cc.getReturnLoc(typeTable->i64(), &callState);
    EXPECT_EQ(retLoc.getType(), ArgLocationType::Register);

    // Verify callee saved registers
    const auto &calleeSaved = cc.getAllCalleeSavedRegs();
    EXPECT_EQ(calleeSaved.size(), 7); // rbx, rsp, rbp, r12, r13, r14, r15
}

TEST_F(MirAbiLowererTest, TestWin64CallingConvention)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *gprClass = getTargetDesc()->getGprClass();
    auto *func = createTestFunction("test_win64_func", typeTable->i64());

    Win64CallingConvDesc cc(ctx, gprClass);
    EXPECT_STREQ(cc.getName(), "Win64");
    EXPECT_EQ(cc.getStackAlignment(), 16);
    EXPECT_EQ(cc.getShadowSpaceSize(), 32);
    EXPECT_EQ(cc.getRedZoneSize(), 0);
    EXPECT_FALSE(cc.isCalleeCleanup());
    EXPECT_TRUE(cc.doesStackGrowsDownwards());
    EXPECT_TRUE(cc.consumesSretSlot());

    CallLoweringState callState(&cc, ctx, func);

    // 6 arguments of i64:
    // First 4 must be passed in unified slots (rcx, rdx, r8, r9)
    for (size_t i = 0; i < 4; ++i)
    {
        auto loc = cc.getArgLoc(typeTable->i64(), &callState);
        EXPECT_EQ(loc.getType(), ArgLocationType::Register);
    }
    EXPECT_EQ(callState.getSlotIndex(), 4);

    // Remaining 2 arguments must be on stack
    auto stackLoc1 = cc.getArgLoc(typeTable->i64(), &callState);
    EXPECT_EQ(stackLoc1.getType(), ArgLocationType::Stack);
    EXPECT_EQ(stackLoc1.getStack().m_sizeBytes, 8);

    auto stackLoc2 = cc.getArgLoc(typeTable->i64(), &callState);
    EXPECT_EQ(stackLoc2.getType(), ArgLocationType::Stack);
    EXPECT_EQ(stackLoc2.getStack().m_sizeBytes, 8);

    // Verify return in RAX
    auto retLoc = cc.getReturnLoc(typeTable->i64(), &callState);
    EXPECT_EQ(retLoc.getType(), ArgLocationType::Register);

    // Verify callee saved registers
    const auto &calleeSaved = cc.getAllCalleeSavedRegs();
    EXPECT_EQ(calleeSaved.size(), 8); // rbx, rbp, rdi, rsi, r12, r13, r14, r15
}
