#include <gtest/gtest.h>
#include "EzMirTestSuite.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionSet.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"
#include "Parser/MirParser.h"
#include "Printer/MirPrinter.h"
#include "MirPasses/Passes/MirVerifierPass.h"

class MirExceptionsTest : public MirTestSuiteAsGtest
{
  protected:
    MirBlock *createNamedBlock(const char *name)
    {
        MirFunction *func = getTestFunc();
        MirBlockBuilder builder(getBuilderCtx(), func);
        return builder.build(nullptr, name);
    }
};

/**
 * T_EXC_01: Verifies static metadata and opcode declarations for TRY, CATCH, THROW.
 */
TEST_F(MirExceptionsTest, T_EXC_01_MetadataAndFlags)
{
    // 1. TRY instruction metadata
    const MirInstructionMetadata &tryMeta = getMeta(MirInstructionOpCode::TRY);
    EXPECT_EQ(tryMeta.m_opcode, MirInstructionOpCode::TRY);
    EXPECT_EQ(tryMeta.m_category, MirInstructionCategory::MirCat_ControlFlow);
    EXPECT_TRUE(tryMeta.m_flags & MirInstructionFlags::IsTerminator);
    EXPECT_TRUE(tryMeta.m_flags & MirInstructionFlags::IsBranch);
    EXPECT_TRUE(tryMeta.m_flags & MirInstructionFlags::HasSideEffect);
    EXPECT_EQ(tryMeta.m_operandMeta.m_count, 2);
    EXPECT_EQ(tryMeta.m_operandMeta.m_slots[0].type, ExpectedOperandType::Reference);
    EXPECT_EQ(tryMeta.m_operandMeta.m_slots[1].type, ExpectedOperandType::Reference);

    // 2. CATCH instruction metadata
    const MirInstructionMetadata &catchMeta = getMeta(MirInstructionOpCode::CATCH);
    EXPECT_EQ(catchMeta.m_opcode, MirInstructionOpCode::CATCH);
    EXPECT_EQ(catchMeta.m_category, MirInstructionCategory::MirCat_ControlFlow);
    EXPECT_TRUE(catchMeta.m_flags & MirInstructionFlags::HasSideEffect);
    EXPECT_TRUE(catchMeta.m_flags & MirInstructionFlags::VariadicArgs);
    EXPECT_EQ(catchMeta.m_operandMeta.m_count, 2);
    EXPECT_EQ(catchMeta.m_operandMeta.m_slots[0].type, ExpectedOperandType::Register);
    EXPECT_EQ(catchMeta.m_operandMeta.m_slots[0].flags, MirOperandFlag::Write);
    EXPECT_EQ(catchMeta.m_operandMeta.m_slots[1].type, ExpectedOperandType::VariadicArgs);

    // 3. THROW instruction metadata
    const MirInstructionMetadata &throwMeta = getMeta(MirInstructionOpCode::THROW);
    EXPECT_EQ(throwMeta.m_opcode, MirInstructionOpCode::THROW);
    EXPECT_EQ(throwMeta.m_category, MirInstructionCategory::MirCat_ControlFlow);
    EXPECT_TRUE(throwMeta.m_flags & MirInstructionFlags::IsTerminator);
    EXPECT_TRUE(throwMeta.m_flags & MirInstructionFlags::HasSideEffect);
    EXPECT_TRUE(throwMeta.m_flags & MirInstructionFlags::VariadicArgs);
    EXPECT_EQ(throwMeta.m_operandMeta.m_count, 2);
    EXPECT_EQ(throwMeta.m_operandMeta.m_slots[0].type, ExpectedOperandType::RegImm);
    EXPECT_EQ(throwMeta.m_operandMeta.m_slots[1].type, ExpectedOperandType::VariadicArgs);
}

/**
 * T_EXC_02: Programmatic building of TRY, CATCH, and THROW instructions in a function.
 */
TEST_F(MirExceptionsTest, T_EXC_02_InstructionBuilding)
{
    auto *ctx = getBuilderCtx();
    auto *func = getTestFunc();
    auto *entryBlk = func->getEntryPoint();

    MirBlock *bodyBlk = createNamedBlock("body");
    MirBlock *catchBlk = createNamedBlock("catch_handler");

    MirOperandBuilder opBuilder(ctx);

    // Build TRY in entry block
    MirInstructionBuilder entryIb(ctx, entryBlk, InsertionType::Append);
    MirOperand *bodyRef = opBuilder.buildRef(bodyBlk);
    MirOperand *catchRef = opBuilder.buildRef(catchBlk);
    MirInstruction *tryInst = entryIb.build(MirInstructionOpCode::TRY, nullptr, { bodyRef, catchRef });
    ASSERT_NE(tryInst, nullptr);
    EXPECT_EQ(tryInst->getOpCode(), MirInstructionOpCode::TRY);
    EXPECT_EQ(tryInst->getOperandCount(), 2);

    // Build THROW in body block
    MirInstructionBuilder bodyIb(ctx, bodyBlk, InsertionType::Append);
    MirOperand *errCode = opBuilder.buildInt(ctx->getTypeTable()->i64(), FlexInt(500, 64));
    MirInstruction *throwInst = bodyIb.build(MirInstructionOpCode::THROW, nullptr, { errCode });
    ASSERT_NE(throwInst, nullptr);
    EXPECT_EQ(throwInst->getOpCode(), MirInstructionOpCode::THROW);
    EXPECT_EQ(throwInst->getOperandCount(), 1);

    // Build CATCH in catch block
    MirInstructionBuilder catchIb(ctx, catchBlk, InsertionType::Append);
    MirRegister *caughtReg = opBuilder.buildVReg(ctx->getTypeTable()->i64(), "ex_val");
    MirInstruction *catchInst = catchIb.build(MirInstructionOpCode::CATCH, nullptr, { caughtReg });
    ASSERT_NE(catchInst, nullptr);
    EXPECT_EQ(catchInst->getOpCode(), MirInstructionOpCode::CATCH);
    EXPECT_EQ(catchInst->getOperandCount(), 1);

    // Return from catch block
    MirInstruction *retInst = catchIb.build(MirInstructionOpCode::RET, nullptr, { caughtReg });
    ASSERT_NE(retInst, nullptr);

    // Run verifier pass
    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_TRUE(pass->getResult().isValid());
}

/**
 * T_EXC_03: Parsing .mir text containing TRY, CATCH, and THROW statements.
 */
TEST_F(MirExceptionsTest, T_EXC_03_MirParsingAndRoundTrip)
{
    MirBuilderContext *ctx = getBuilderCtx();
    EzMir::MirParser parser(ctx);

    std::string_view mirSource = R"mir(
fn @exception_flow() -> i64 {
entry:
    TRY label %protected_area, label %handle_error;

protected_area:
    %err = MOV i64 123;
    THROW i64 %err;

handle_error:
    %ex = CATCH i64;
    RET i64 %ex;
}
)mir";

    bool parsed = parser.parseModule(mirSource, "exception_flow.mir");
    ASSERT_TRUE(parsed);

    MirFunction *fn = nullptr;
    for (MirFunction *f : ctx->getFunctions())
    {
        if (f->getName() == "exception_flow")
        {
            fn = f;
            break;
        }
    }
    ASSERT_NE(fn, nullptr);
    std::vector<MirBlock *> blocks;
    for (MirBlock *b : fn->getBlocks())
    {
        blocks.push_back(b);
    }
    ASSERT_EQ(blocks.size(), 3);

    // Check entry block's terminator
    MirBlock *entryBlk = blocks[0];
    ASSERT_NE(entryBlk, nullptr);
    ASSERT_FALSE(entryBlk->getInstructions().empty());
    MirInstruction *tryInst = entryBlk->getInstructions().back();
    EXPECT_EQ(tryInst->getOpCode(), MirInstructionOpCode::TRY);
    EXPECT_EQ(tryInst->getOperandCount(), 2);

    // Check catch block
    MirBlock *catchBlk = blocks[2];
    ASSERT_NE(catchBlk, nullptr);
    ASSERT_FALSE(catchBlk->getInstructions().empty());
    MirInstruction *catchInst = catchBlk->getInstructions().front();
    EXPECT_EQ(catchInst->getOpCode(), MirInstructionOpCode::CATCH);

    // Run verifier pass
    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_TRUE(pass->getResult().isValid());

    // Test formatting through MirPrinter
    std::string printed = MirPrinter::printFunction(fn, MirPrinterMode::Parseable);
    EXPECT_NE(printed.find("TRY"), std::string::npos);
    EXPECT_NE(printed.find("THROW"), std::string::npos);
    EXPECT_NE(printed.find("CATCH"), std::string::npos);
}

/**
 * T_EXC_04: Negative verification tests for malformed exception instructions.
 */
TEST_F(MirExceptionsTest, T_EXC_04_NegativeVerification)
{
    auto *ctx = getBuilderCtx();
    auto *func = getTestFunc();
    auto *entryBlk = func->getEntryPoint();

    MirOperandBuilder opBuilder(ctx);
    MirInstructionBuilder ib(ctx, entryBlk, InsertionType::Append);

    // TRY with only 1 operand (requires 2 block references)
    MirOperand *bodyRef = opBuilder.buildRef(entryBlk);
    MirInstruction *badTry = ib.build(MirInstructionOpCode::TRY, nullptr, { bodyRef });
    ASSERT_NE(badTry, nullptr);

    auto *pass = runPass<MirVerifierPass>(ctx);
    ASSERT_NE(pass, nullptr);
    EXPECT_FALSE(pass->getResult().isValid());
}
