#include "EzTripleTestSuite.h"
#include "Block/MirBlock.h"
#include "Function/MirFunctionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Legalizer/Actions/LegalizeBitcastAction.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeLibcallAction.h"
#include "Legalizer/Actions/LegalizeNarrowScalarAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/Actions/LegalizeWidenScalarAction.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Legalizer/MirLegalizer.h"
#include "Legalizer/MirLegalizerPass.h"
#include "Operand/MirOperandBuilder.h"

class MirFrameLowererTest : public EzTripleTestSuite
{
};

// ============================================================================
// 1. Calling Convention & Function Signature Tests
// ============================================================================

TEST_F(MirFrameLowererTest, TestFunctionSignatureLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("add_params", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirOperandBuilder opBuilder(ctx);
    MirRegister *p0 = opBuilder.buildVReg(typeTable->i32(), "a");
    MirRegister *p1 = opBuilder.buildVReg(typeTable->i32(), "b");
    MirFunctionBuilder(ctx).addParam(func, p0).addParam(func, p1);

    MirFunctionSignatureLegalizerPass sigPass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = sigPass.run(funcList.begin(), nullptr);
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

TEST_F(MirFrameLowererTest, TestSretFunctionSignatureLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    // 256-bit return type exceeds register return capacity -> requires SRET
    auto *func = createTestFunction("sret_func", typeTable->i256());

    MirOperandBuilder opBuilder(ctx);
    MirRegister *p0 = opBuilder.buildVReg(typeTable->i32(), "val");
    MirFunctionBuilder(ctx).addParam(func, p0);

    MirFunctionSignatureLegalizerPass sigPass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = sigPass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
    EXPECT_TRUE(result.m_modifiedMir);

    // Verify SRET pointer was prepended to parameters
    EXPECT_EQ(func->getParameters().size(), 2);
    MirRegister *sretPtr = func->getParameters().front();
    EXPECT_EQ(sretPtr->getMirType()->getKind(), MirTypeKind::Pointer);
}

TEST_F(MirFrameLowererTest, TestCallLegalization)
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

TEST_F(MirFrameLowererTest, TestReturnLegalization)
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

// ============================================================================
// 2. Action Direct-Invocation Tests
// ============================================================================

TEST_F(MirFrameLowererTest, TestWidenScalarLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("widen_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i8(), "dst");
    MirRegister *lhs = ob.buildVReg(typeTable->i8(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i8(), "rhs");

    ib.ADD(dst, lhs, rhs);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeWidenScalar(legCtx, 0, typeTable->i32());
    EXPECT_EQ(res, LegalizationResult::Legalized);
}

TEST_F(MirFrameLowererTest, TestNarrowScalarLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("narrow_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i64(), "dst");
    MirRegister *lhs = ob.buildVReg(typeTable->i64(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i64(), "rhs");

    ib.ADD(dst, lhs, rhs);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeNarrowScalar(legCtx, 0, typeTable->i32());
    EXPECT_EQ(res, LegalizationResult::Legalized);
}

TEST_F(MirFrameLowererTest, TestBitcastLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("bitcast_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i32(), "dst");
    MirRegister *src = ob.buildVReg(typeTable->f32(), "src");

    ib.MOV(dst, src);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeBitcast(legCtx, 1, typeTable->i32());
    EXPECT_EQ(res, LegalizationResult::Legalized);
}

TEST_F(MirFrameLowererTest, TestLibcallLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("libcall_test", typeTable->i64());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i64(), "dst");
    MirRegister *lhs = ob.buildVReg(typeTable->i64(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i64(), "rhs");

    ib.DIV(dst, lhs, rhs);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeLibcall(legCtx, "__divdi3");
    EXPECT_EQ(res, LegalizationResult::Legalized);
}

TEST_F(MirFrameLowererTest, TestWidenCompareLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("widen_cmp_test", typeTable->i1());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i1(), "cond");
    MirRegister *lhs = ob.buildVReg(typeTable->i8(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i8(), "rhs");

    ib.CMP_EQ(dst, lhs, rhs);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeWidenScalar(legCtx, 0, typeTable->i32());
    EXPECT_EQ(res, LegalizationResult::Legalized);

    // Verify CMP_EQ dst is still i1 and inputs were widened
    auto &instructions = block->getInstructions();
    EXPECT_EQ(instructions.size(), 3); // ZEXT lhs, ZEXT rhs, CMP_EQ dst
}

TEST_F(MirFrameLowererTest, TestWidenSignedArithmeticLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("widen_signed_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i8(), "dst");
    MirRegister *lhs = ob.buildVReg(typeTable->i8(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i8(), "rhs");

    ib.IDIV(dst, lhs, rhs);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeWidenScalar(legCtx, 0, typeTable->i32());
    EXPECT_EQ(res, LegalizationResult::Legalized);

    auto &instructions = block->getInstructions();
    auto cur = instructions.begin();
    MirInstruction *sext1 = *cur++;
    MirInstruction *sext2 = *cur++;
    MirInstruction *idivInst = *cur++;
    MirInstruction *truncInst = *cur++;

    EXPECT_EQ(sext1->getOpCodeName(), std::string("SEXT"));
    EXPECT_EQ(sext2->getOpCodeName(), std::string("SEXT"));
    EXPECT_EQ(idivInst->getOpCodeName(), std::string("IDIV"));
    EXPECT_EQ(truncInst->getOpCodeName(), std::string("TRUNC"));
}

TEST_F(MirFrameLowererTest, TestNarrowSubAndNegLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("narrow_sub_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i128(), "dst");
    MirRegister *lhs = ob.buildVReg(typeTable->i128(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i128(), "rhs");

    ib.SUB(dst, lhs, rhs);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeNarrowScalar(legCtx, 0, typeTable->i64());
    EXPECT_EQ(res, LegalizationResult::Legalized);

    // Verify SUB was lowered to UNMERGE x2, USUBO, USUBE, MERGE
    auto &instructions = block->getInstructions();
    EXPECT_EQ(instructions.size(), 5);
}

TEST_F(MirFrameLowererTest, TestNarrowBitwiseLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("narrow_bitwise_test", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i128(), "dst");
    MirRegister *lhs = ob.buildVReg(typeTable->i128(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i128(), "rhs");

    ib.XOR(dst, lhs, rhs);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeNarrowScalar(legCtx, 0, typeTable->i64());
    EXPECT_EQ(res, LegalizationResult::Legalized);

    // Verify XOR was lowered to UNMERGE x2, XOR x2, MERGE
    auto &instructions = block->getInstructions();
    EXPECT_EQ(instructions.size(), 5);
}

TEST_F(MirFrameLowererTest, TestNarrowCompareLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("narrow_cmp_test", typeTable->i1());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *dst = ob.buildVReg(typeTable->i1(), "eq");
    MirRegister *lhs = ob.buildVReg(typeTable->i128(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i128(), "rhs");

    ib.CMP_EQ(dst, lhs, rhs);

    auto it = block->getInstructions().begin();
    LegalizeCtx legCtx(ctx, getTargetDesc(), it);
    auto res = LegalizeActions::LegalizeNarrowScalar(legCtx, 0, typeTable->i64());
    EXPECT_EQ(res, LegalizationResult::Legalized);

    // Verify CMP_EQ lowered to UNMERGE x2, CMP_EQ x2, AND, MOV dst
    auto &instructions = block->getInstructions();
    EXPECT_EQ(instructions.size(), 6);
}

// ============================================================================
// 3. End-to-End Table-Driven Driver Tests (Phase 3 LUT Integration)
// ============================================================================

TEST_F(MirFrameLowererTest, TestTableDrivenWidenInstruction)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("table_widen", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // ADD %v0:i8, %v1:i8, %v2:i8 -> triggers WidenScalar to i32 via Action Table
    MirRegister *dst = ob.buildVReg(typeTable->i8(), "dst");
    MirRegister *lhs = ob.buildVReg(typeTable->i8(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i8(), "rhs");
    ib.ADD(dst, lhs, rhs);

    MirLegalizer legalizer(ctx, getTargetDesc());
    bool success = legalizer.legalizeBlock(block);
    EXPECT_TRUE(success);

    // Result should contain ZEXT x2, ADD:i32, TRUNC:i8
    EXPECT_GT(block->getInstructions().size(), 1);
}

TEST_F(MirFrameLowererTest, TestTableDrivenMultiSlotHeterogeneousLegalization)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("table_hetero_sext", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // SEXT %dst:i32, %src:i1 -> triggers WidenScalar on Slot 1 (i1 -> i8)
    MirRegister *dst = ob.buildVReg(typeTable->i32(), "dst");
    MirRegister *src = ob.buildVReg(typeTable->i1(), "src");
    ib.SEXT(dst, src);

    MirLegalizer legalizer(ctx, getTargetDesc());
    bool success = legalizer.legalizeBlock(block);
    EXPECT_TRUE(success);

    // Verify SEXT src was widened to legal (i32, i8) pair
    auto *legalSext = block->getInstructions().back();
    EXPECT_EQ(legalSext->getOpCodeName(), std::string("SEXT"));
    EXPECT_EQ(legalSext->getOperand(0)->getMirType()->getKind(), MirTypeKind::Integer);
    EXPECT_EQ(legalSext->getOperand(0)->getSizeInBytes(), 4); // i32
    EXPECT_EQ(legalSext->getOperand(1)->getSizeInBytes(), 1); // i8
}

TEST_F(MirFrameLowererTest, TestTableDrivenLibcallInstruction)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("table_libcall", typeTable->i64());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    // DIV:i64 -> triggers Libcall "__divdi3" via Action Table
    MirRegister *dst = ob.buildVReg(typeTable->i64(), "dst");
    MirRegister *lhs = ob.buildVReg(typeTable->i64(), "lhs");
    MirRegister *rhs = ob.buildVReg(typeTable->i64(), "rhs");
    ib.DIV(dst, lhs, rhs);

    MirLegalizer legalizer(ctx, getTargetDesc());
    bool success = legalizer.legalizeBlock(block);
    EXPECT_TRUE(success);

    // Verify CALL was emitted for runtime symbol
    bool hasCall = false;
    for (auto *inst : block->getInstructions())
    {
        if (inst->getFlags() & MirInstructionFlags::IsCall)
        {
            hasCall = true;
            break;
        }
    }
    EXPECT_TRUE(hasCall);
}

TEST_F(MirFrameLowererTest, TestFullLegalizerPass)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = ctx->getTypeTable();
    auto *func = createTestFunction("full_legalize", typeTable->i32());
    auto *block = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, block, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    MirRegister *p0 = ob.buildVReg(typeTable->i32(), "x");
    MirFunctionBuilder(ctx).addParam(func, p0);

    MirRegister *retVal = ob.buildVReg(typeTable->i32(), "y");
    ib.RET(retVal);

    MirLegalizerPass pass(ctx, getTargetDesc());
    IntrusiveLinkedList<MirFunction> funcList;
    funcList.push_back(func);

    auto result = pass.run(funcList.begin(), nullptr);
    EXPECT_TRUE(result.m_succeeded);
    EXPECT_TRUE(result.m_modifiedMir);
}