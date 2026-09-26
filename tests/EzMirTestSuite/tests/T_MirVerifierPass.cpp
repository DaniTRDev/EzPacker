#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "MirPasses/Passes/MirVerifierPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

/**
 * Test fixture for the MIR verifier analysis pass.
 */
class MirVerifierPassTest : public MirTestSuiteAsGtest
{
  protected:
    MirBlock *createBlock(const char *name)
    {
        MirFunction *func = getTestFunc();
        MirBlockBuilder builder(getBuilderCtx(), func);
        return builder.build(nullptr, name);
    }
};

TEST_F(MirVerifierPassTest, TestValidInstructions)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *i64Type = typeTable->i64();
    auto *i1Type = typeTable->i1();

    auto *dst32 = ob.buildVReg(i32Type, "dst32");
    auto *lhs32 = ob.buildVReg(i32Type, "lhs32");
    auto *rhs32 = ob.buildVReg(i32Type, "rhs32");
    auto *imm32 = ob.buildInt(i32Type, FlexInt(42, 32));

    auto *dst64 = ob.buildVReg(i64Type, "dst64");
    auto *src32 = ob.buildVReg(i32Type, "src32");
    auto *cmpDst = ob.buildVReg(i1Type, "cmpDst");

    // Valid ALU and moves
    ib.ADD(dst32, lhs32, rhs32);
    ib.SUB(dst32, lhs32, imm32);
    ib.CMP_EQ(cmpDst, lhs32, rhs32);
    ib.ZEXT(dst64, src32);
    ib.TRUNC(src32, dst64);
    ib.RET(dst32);

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_TRUE(pass->getResult().isValid());
    EXPECT_EQ(pass->getResult().m_errorCount, 0);
    EXPECT_GT(pass->getResult().m_instructionCount, 0);
}

TEST_F(MirVerifierPassTest, TestSizeMatchViolation_Arithmetic)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *i64Type = typeTable->i64();

    auto *dst32 = ob.buildVReg(i32Type, "dst");
    auto *lhs64 = ob.buildVReg(i64Type, "lhs");
    auto *rhs32 = ob.buildVReg(i32Type, "rhs");

    // ADD with mismatched operand bit-widths (32 vs 64)
    ib.ADD(dst32, lhs64, rhs32);

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
    EXPECT_GT(pass->getResult().m_errorCount, 0);
}

TEST_F(MirVerifierPassTest, TestSizeMatchViolation_Comparison)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i1Type = typeTable->i1();
    auto *i32Type = typeTable->i32();
    auto *i64Type = typeTable->i64();

    auto *cmpDst = ob.buildVReg(i1Type, "cond");
    auto *lhs32 = ob.buildVReg(i32Type, "lhs");
    auto *rhs64 = ob.buildVReg(i64Type, "rhs");

    // CMP_SLT comparing 32-bit lhs against 64-bit rhs
    ib.CMP_SLT(cmpDst, lhs32, rhs64);

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
    EXPECT_GT(pass->getResult().m_errorCount, 0);
}

TEST_F(MirVerifierPassTest, TestDestLargerViolation)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i16Type = typeTable->i16();
    auto *i32Type = typeTable->i32();

    auto *dst16 = ob.buildVReg(i16Type, "dst16");
    auto *src32 = ob.buildVReg(i32Type, "src32");

    // ZEXT where destination (16 bits) is smaller than source (32 bits)
    ib.ZEXT(dst16, src32);

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
    EXPECT_GT(pass->getResult().m_errorCount, 0);
}

TEST_F(MirVerifierPassTest, TestDestSmallerViolation)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i64Type = typeTable->i64();
    auto *i32Type = typeTable->i32();

    auto *dst64 = ob.buildVReg(i64Type, "dst64");
    auto *src32 = ob.buildVReg(i32Type, "src32");

    // TRUNC where destination (64 bits) is larger than source (32 bits)
    ib.TRUNC(dst64, src32);

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
    EXPECT_GT(pass->getResult().m_errorCount, 0);
}

TEST_F(MirVerifierPassTest, TestOperandKindMismatch_DefMustBeRegister)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *imm = ob.buildInt(i32Type, FlexInt(10, 32));
    auto *lhs = ob.buildVReg(i32Type, "lhs");
    auto *rhs = ob.buildVReg(i32Type, "rhs");

    // Attempt to emit ADD writing into an immediate constant
    ib.build(MirInstructionOpCode::ADD, nullptr, { imm, lhs, rhs });

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
    EXPECT_GT(pass->getResult().m_errorCount, 0);
}

TEST_F(MirVerifierPassTest, TestOperandCountMismatch)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *dst = ob.buildVReg(i32Type, "dst");
    auto *lhs = ob.buildVReg(i32Type, "lhs");

    // Binary ADD with only 2 operands instead of 3
    ib.build(MirInstructionOpCode::ADD, nullptr, { dst, lhs });

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
    EXPECT_GT(pass->getResult().m_errorCount, 0);
}

TEST_F(MirVerifierPassTest, TestTypeConsistency_SignedFloatMismatch)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *f32Type = typeTable->f32();
    auto *dst = ob.buildVReg(f32Type, "dst");
    auto *lhs = ob.buildVReg(f32Type, "lhs");
    auto *rhs = ob.buildVReg(f32Type, "rhs");

    // SDIV has TreatAsSigned flag, should reject float operands
    ib.SDIV(dst, lhs, rhs);

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
    EXPECT_GT(pass->getResult().m_errorCount, 0);
}

TEST_F(MirVerifierPassTest, TestTypeConsistency_PureFloatWithIntMismatch)
{
    auto *ctx = getBuilderCtx();
    auto *typeTable = getTypeTable();
    auto *func = getTestFunc();
    auto *entry = func->getEntryPoint();

    MirInstructionBuilder ib(ctx, entry, InsertionType::Append);
    MirOperandBuilder ob(ctx);

    auto *i32Type = typeTable->i32();
    auto *dst = ob.buildVReg(i32Type, "dst");
    auto *lhs = ob.buildVReg(i32Type, "lhs");
    auto *rhs = ob.buildVReg(i32Type, "rhs");

    // FADD expects floating-point operands, should reject integer operands
    ib.FADD(dst, lhs, rhs);

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
    EXPECT_GT(pass->getResult().m_errorCount, 0);
}
