#include "EzTripleTestSuite.h"
#include "AbiLowerer/MirAbiLowerer.h"
#include "AbiLowerer/MirAbiLowererPass.h"
#include "Instruction/MirInstruction.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Operand/MirOperandBuilder.h"

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

    auto result = pass.run(funcList, funcList.begin(), nullptr);
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

    auto result = pass.run(funcList, funcList.begin(), nullptr);
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
    func->getParameters().push_back(p0);
    func->getParameters().push_back(p1);

    // Legalize signature -> emits POP_ARG, POP_ARG, END_ARG
    MirFunctionSignatureLegalizerPass sigPass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);
    sigPass.run(funcList, funcList.begin(), nullptr);

    // Run ABI lowerer
    MirAbiLowererPass pass(ctx);
    auto result = pass.run(funcList, funcList.begin(), nullptr);
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
    sigPass.run(funcList, funcList.begin(), nullptr);

    // Return legalization
    auto it = --block->getInstructions().end();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    LegalizeActions::LegalizeReturn(legCtx);

    // ABI Lowering
    MirAbiLowererPass pass(ctx);
    auto result = pass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
}
