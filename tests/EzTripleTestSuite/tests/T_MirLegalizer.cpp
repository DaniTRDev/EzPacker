#include "EzTripleTestSuite.h"
#include "Instruction/MirInstruction.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Legalizer/MirLegalizer.h"
#include "Legalizer/MirLegalizerPass.h"
#include "Operand/MirOperandBuilder.h"

class MirLegalizerTest : public EzTripleTestSuite
{
};

TEST_F(MirLegalizerTest, TestFunctionSignatureLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("add_params", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirOperandBuilder opBuilder(ctx);
    MirRegister *p0 = opBuilder.buildVReg(typeTable->i32(), "a");
    MirRegister *p1 = opBuilder.buildVReg(typeTable->i32(), "b");
    func->getParameters().push_back(p0);
    func->getParameters().push_back(p1);

    MirFunctionSignatureLegalizerPass sigPass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = sigPass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
    EXPECT_TRUE(result.m_modifiedMir);

    // Verify entry block contains POP_ARG, POP_ARG, END_ARG
    auto &instructions = block->getInstructions();
    EXPECT_EQ(instructions.size(), 3);

    auto it = instructions.begin();
    MirInstruction *pop0 = *it++;
    MirInstruction *pop1 = *it++;
    MirInstruction *endArg = *it++;

    EXPECT_EQ(pop0->getOpCodeName(), std::string("POP_ARG"));
    EXPECT_EQ(pop1->getOpCodeName(), std::string("POP_ARG"));
    EXPECT_EQ(endArg->getOpCodeName(), std::string("END_ARG"));
}

TEST_F(MirLegalizerTest, TestSretFunctionSignatureLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    // 256-bit return type exceeds register return capacity -> requires SRET
    auto *func = createTestFunction("sret_func", typeTable->i256());

    MirOperandBuilder opBuilder(ctx);
    MirRegister *p0 = opBuilder.buildVReg(typeTable->i32(), "val");
    func->getParameters().push_back(p0);

    MirFunctionSignatureLegalizerPass sigPass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = sigPass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
    EXPECT_TRUE(result.m_modifiedMir);

    // Verify SRET pointer was prepended to parameters
    EXPECT_EQ(func->getParameters().size(), 2);
    MirRegister *sretPtr = func->getParameters().front();
    EXPECT_EQ(sretPtr->getMirType()->getKind(), MirTypeKind::Pointer);
}

TEST_F(MirLegalizerTest, TestCallLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("caller", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dest = ob.buildVReg(typeTable->i32(), "ret_val");
    MirRegister *arg0 = ob.buildVReg(typeTable->i32(), "arg0");
    MirReference *callee = ob.buildRef(func);

    // Emit generic CALL: dest = CALL callee, arg0
    ib.CALL(dest, callee, arg0);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeCall(legCtx);
    EXPECT_EQ(res, LegalizationResult::Legalized);

    // Check that PUSH_ARG was inserted before CALL and POP_RET after CALL
    auto &instList = block->getInstructions();
    EXPECT_EQ(instList.size(), 3);

    auto cur = instList.begin();
    MirInstruction *pushInst = *cur++;
    MirInstruction *callInst = *cur++;
    MirInstruction *popInst = *cur++;

    EXPECT_EQ(pushInst->getOpCodeName(), std::string("PUSH_ARG"));
    EXPECT_EQ(callInst->getOpCodeName(), std::string("CALL"));
    EXPECT_EQ(popInst->getOpCodeName(), std::string("POP_RET"));
}

TEST_F(MirLegalizerTest, TestReturnLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("returner", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *retVal = ob.buildVReg(typeTable->i32(), "res");
    ib.RET(retVal);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeReturn(legCtx);
    EXPECT_EQ(res, LegalizationResult::Legalized);

    // Check that PUSH_RET was emitted and RET now uses the binding token
    auto &instList = block->getInstructions();
    EXPECT_EQ(instList.size(), 2);

    auto cur = instList.begin();
    MirInstruction *pushRet = *cur++;
    MirInstruction *retInst = *cur++;

    EXPECT_EQ(pushRet->getOpCodeName(), std::string("PUSH_RET"));
    EXPECT_EQ(retInst->getOpCodeName(), std::string("RET"));
}

TEST_F(MirLegalizerTest, TestFullLegalizerPass)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("full_legalize", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *p0 = ob.buildVReg(typeTable->i32(), "x");
    func->getParameters().push_back(p0);

    MirRegister *retVal = ob.buildVReg(typeTable->i32(), "y");
    ib.RET(retVal);

    MirLegalizerPass pass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = pass.run(funcList, funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
    EXPECT_TRUE(result.m_modifiedMir);
}
