#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/NonSsaToSsaPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"
#include "Type/MirTypeTable.h"

class NonSsaToSsaTest : public MirTestSuiteAsGtest
{
  protected:
    void runSSAPass()
    {
        MirPassManager *passManager = getPassManager();
        // Register passes
        passManager->addPass<CodeFlowAnalysisPass>(getBuilderCtx());
        passManager->addPass<NonSsaToSsaPass>(getBuilderCtx());

        // Now run them:
        passManager->generatePipeline();
        passManager->runPipeline(getBuilderCtx());
    }

    // Helper to abstract away block creation if your API differs slightly
    MirBlock *createBlock(const char *name)
    {
        MirFunction *func = getTestFunc();
        MirBlockBuilder builder(getBuilderCtx(), func);

        return builder.build(nullptr, name);
    }
};

namespace
{
::testing::AssertionResult IsPhi(MirInstruction *instr, size_t expectedIncomingPaths)
{
    if (!instr)
        return ::testing::AssertionFailure() << "Instruction is nullptr";

    if (!instr->hasOpcode(MirInstructionOpCode::PHI))
        return ::testing::AssertionFailure() << "Expected PHI opcode, got " << instr->getOpCodeName();

    // Operand 0 is the destination, the rest are incoming values
    if (instr->getOperandCount() != expectedIncomingPaths + 1)
        return ::testing::AssertionFailure()
                << "Expected " << expectedIncomingPaths << " incoming paths, got " << (instr->getOperandCount() - 1);

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult IsUndef(MirOperand *op)
{
    if (!op)
        return ::testing::AssertionFailure() << "Operand is nullptr";
    if (op->getType() != MirOperandType::Register)
        return ::testing::AssertionFailure() << "Expected Register";

    MirRegister *reg = static_cast<MirRegister *>(op);
    if (reg->getName() != "undef")
        return ::testing::AssertionFailure() << "Expected 'undef' register, got " << reg->getName();

    return ::testing::AssertionSuccess();
}
} // anonymous namespace

// =========================================================================
// TEST 1: Straight-line code renaming (No PHI nodes expected)
// =========================================================================
TEST_F(NonSsaToSsaTest, StraightLineRenaming)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    MirType *i32 = getTypeTable()->i32();

    MirRegister *varX = oBuilder.buildVReg(i32, "x");
    MirRegister *varY = oBuilder.buildVReg(i32, "y");

    // Block: Entry
    // x = 1
    MirInstruction *def1 = iBuilder.MOV(varX, oBuilder.buildInt(i32, FlexInt(1)));
    // x = 2
    MirInstruction *def2 = iBuilder.MOV(varX, oBuilder.buildInt(i32, FlexInt(2)));
    // y = x
    MirInstruction *useInst = iBuilder.MOV(varY, varX);

    runSSAPass();

    // Verify
    MirRegister *xDef1 = def1->getOpAs<MirRegister>(0);
    MirRegister *xDef2 = def2->getOpAs<MirRegister>(0);
    MirRegister *xUse = useInst->getOpAs<MirRegister>(1);

    // Ensure versions are distinct
    EXPECT_NE(xDef1->getRegId(), xDef2->getRegId()) << "Registers were not properly renamed/versioned.";

    // Ensure the use maps to the LATEST definition
    EXPECT_EQ(xDef2->getRegId(), xUse->getRegId())
            << "Read operand did not map to the most recent dominance definition.";
}

// =========================================================================
// TEST 2: Diamond CFG (If-Then-Else) -> Expect 1 PHI node
// =========================================================================
TEST_F(NonSsaToSsaTest, DiamondCfgPhiInsertion)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    MirType *i32 = getBuilderCtx()->getTypeTable()->i32();
    MirFunction *func = getTestFunc();

    MirBlock *entry = func->getEntryPoint();
    MirBlock *thenB = createBlock("then");
    MirBlock *elseB = createBlock("else");
    MirBlock *joinB = createBlock("join");

    MirRegister *varX = oBuilder.buildVReg(i32, "x");
    MirRegister *varY = oBuilder.buildVReg(i32, "y");
    MirRegister *cond = oBuilder.buildVReg(getTypeTable()->i1(), "cond");

    // --- Entry ---
    iBuilder.setInsertionPoint(entry, InsertionType::InsertAfter, entry->begin());
    iBuilder.MOV(varX, oBuilder.buildInt(i32, FlexInt(1)));
    iBuilder.BR_COND(cond, oBuilder.buildRef(thenB), oBuilder.buildRef(elseB)); // Use block references

    // --- Then ---
    iBuilder.setInsertionPoint(thenB, InsertionType::InsertAfter, thenB->begin());
    iBuilder.MOV(varX, oBuilder.buildInt(i32, FlexInt(2)));
    iBuilder.JMP(oBuilder.buildRef(joinB)); // Use block reference

    // --- Else ---
    iBuilder.setInsertionPoint(elseB, InsertionType::InsertAfter, elseB->begin());
    iBuilder.MOV(varX, oBuilder.buildInt(i32, FlexInt(3)));
    iBuilder.JMP(oBuilder.buildRef(joinB)); // Use block reference

    // --- Join ---
    iBuilder.setInsertionPoint(joinB, InsertionType::InsertAfter, joinB->begin());
    MirInstruction *readInst = iBuilder.MOV(varY, varX);

    // Run the SSA construction algorithm
    runSSAPass();

    // Verify PHI insertion in the Join Block
    MirInstruction *phiInst = nullptr;
    for (MirInstruction *inst : joinB->getInstructions())
    {
        if (inst->hasOpcode(MirInstructionOpCode::PHI))
        {
            phiInst = inst;
            break;
        }
    }

    ASSERT_NE(phiInst, nullptr) << "PHI Instruction was NOT inserted at the dominance frontier (Join Block).";
    EXPECT_TRUE(IsPhi(phiInst, 2));

    // Verify that `varY = varX` now reads from the PHI node's destination
    MirRegister *phiDst = phiInst->getOpAs<MirRegister>(0);
    MirRegister *readSrc = readInst->getOpAs<MirRegister>(1);

    EXPECT_EQ(phiDst->getRegId(), readSrc->getRegId()) << "Downstream read instruction should use the PHI result.";
}

// =========================================================================
// TEST 3: Loop CFG (While Loop) -> Expect PHI node at Header
// =========================================================================
TEST_F(NonSsaToSsaTest, LoopHeaderPhiInsertion)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    MirType *i32 = getTypeTable()->i32();

    MirBlock *entry = getTestFunc()->getEntryPoint();
    MirBlock *header = createBlock("header");
    MirBlock *body = createBlock("body");
    MirBlock *exitB = createBlock("exit");

    MirRegister *varX = oBuilder.buildVReg(i32, "x");
    MirRegister *varY = oBuilder.buildVReg(i32, "y");
    MirRegister *cond = oBuilder.buildVReg(getTypeTable()->i1(), "cond");

    // --- Entry ---
    iBuilder.setInsertionPoint(entry, InsertionType::InsertAfter, entry->begin());
    iBuilder.MOV(varX, oBuilder.buildInt(i32, FlexInt(0))); // Initial definition
    iBuilder.JMP(oBuilder.buildRef(header));                // Use block reference

    // --- Header ---
    iBuilder.setInsertionPoint(header, InsertionType::InsertAfter, header->begin());
    iBuilder.BR_COND(cond, oBuilder.buildRef(body), oBuilder.buildRef(exitB)); // Use block references

    // --- Body ---
    iBuilder.setInsertionPoint(body, InsertionType::InsertAfter, body->begin());
    iBuilder.MOV(varX, oBuilder.buildInt(i32, FlexInt(1))); // Override definition
    iBuilder.JMP(oBuilder.buildRef(header));                // Back-edge! Use block reference

    // --- Exit ---
    iBuilder.setInsertionPoint(exitB, InsertionType::InsertAfter, exitB->begin());
    MirInstruction *readInst = iBuilder.MOV(varY, varX); // Use

    runSSAPass();

    // Verify PHI insertion at Header (due to back-edge)
    MirInstruction *phiInst = nullptr;
    for (MirInstruction *inst : header->getInstructions())
    {
        if (inst->hasOpcode(MirInstructionOpCode::PHI))
        {
            phiInst = inst;
            break;
        }
    }

    ASSERT_NE(phiInst, nullptr) << "PHI Instruction was NOT inserted at the Loop Header.";
    EXPECT_TRUE(IsPhi(phiInst, 2)); // Should merge [Entry] and [Body]

    // Verify that the Exit block correctly reads the PHI destination
    // because Header dominates Exit.
    MirRegister *phiDst = phiInst->getOpAs<MirRegister>(0);
    MirRegister *readSrc = readInst->getOpAs<MirRegister>(1);

    EXPECT_EQ(phiDst->getRegId(), readSrc->getRegId());
}

// =========================================================================
// TEST 4: Uninitialized Memory / Undef Variable
// =========================================================================
TEST_F(NonSsaToSsaTest, UndefVariableHandling)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());
    MirType *i32 = getTypeTable()->i32();

    // Variable is never written to!
    MirRegister *varX = oBuilder.buildVReg(i32, "x_uninit");
    MirRegister *varY = oBuilder.buildVReg(i32, "y");

    // --- Entry ---
    // y = x_uninit
    MirInstruction *readInst = iBuilder.MOV(varY, varX);

    runSSAPass();

    // Verify that reading an uninitialized variable generated an 'undef' register
    // rather than throwing a segfault or leaving the original pointer dangling.
    MirRegister *readSrc = readInst->getOpAs<MirRegister>(1);
    EXPECT_TRUE(IsUndef(readSrc));
}