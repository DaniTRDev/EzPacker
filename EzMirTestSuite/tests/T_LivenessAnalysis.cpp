#include <gtest/gtest.h>
#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"

class LivenessAnalysisTest : public MirTestSuiteAsGtest
{
  public:
    // Helper to quickly allocate 32-bit registers in tests
    MirRegister *createInt32Reg(std::string_view name)
    {
        return MirOperandBuilder(getBuilderCtx()).buildVReg(getTypeTable()->i32(), name.data());
    }

    // Helper to quickly allocate 1-bit boolean registers (for BR_COND)
    MirRegister *createInt1Reg(std::string_view name)
    {
        return MirOperandBuilder(getBuilderCtx()).buildVReg(getTypeTable()->i1(), name.data());
    }
};

namespace
{

// ---------------------------------------------------------
// GTest Assertion Helpers
// ---------------------------------------------------------

::testing::AssertionResult CheckSet(const std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>> *map,
                                    size_t blockId,
                                    size_t regId,
                                    bool expected,
                                    const char *setName)
{
    if (!map)
        return ::testing::AssertionFailure() << "Map pointer is null";

    auto it = map->find(blockId);
    bool contains = (it != map->end() && it->second.contains(MirRegisterRef::vreg(regId)));

    if (contains != expected)
    {
        return ::testing::AssertionFailure()
                << "Block " << blockId << (expected ? " MISSING " : " UNEXPECTEDLY CONTAINS ") << "vreg(" << regId
                << ") in its " << setName << " set.";
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult HasLocalDef(const LivenessResult *res, size_t blockId, size_t regId)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";
    return CheckSet(&res->m_def, blockId, regId, true, "DEF");
}

::testing::AssertionResult NotLocalDef(const LivenessResult *res, size_t blockId, size_t regId)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";
    return CheckSet(&res->m_def, blockId, regId, false, "DEF");
}

::testing::AssertionResult HasLocalUse(const LivenessResult *res, size_t blockId, size_t regId)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";
    return CheckSet(&res->m_use, blockId, regId, true, "USE");
}

::testing::AssertionResult NotLocalUse(const LivenessResult *res, size_t blockId, size_t regId)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";
    return CheckSet(&res->m_use, blockId, regId, false, "USE");
}

::testing::AssertionResult IsLiveIn(const LivenessResult *res, size_t blockId, size_t regId)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";
    return CheckSet(&res->m_liveIn, blockId, regId, true, "LIVE-IN");
}

::testing::AssertionResult NotLiveIn(const LivenessResult *res, size_t blockId, size_t regId)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";
    return CheckSet(&res->m_liveIn, blockId, regId, false, "LIVE-IN");
}

::testing::AssertionResult IsLiveOut(const LivenessResult *res, size_t blockId, size_t regId)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";
    return CheckSet(&res->m_liveOut, blockId, regId, true, "LIVE-OUT");
}

::testing::AssertionResult NotLiveOut(const LivenessResult *res, size_t blockId, size_t regId)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";
    return CheckSet(&res->m_liveOut, blockId, regId, false, "LIVE-OUT");
}

::testing::AssertionResult
HasLiveCounts(const LivenessResult *res, size_t blockId, size_t expectedIn, size_t expectedOut)
{
    if (!res)
        return ::testing::AssertionFailure() << "LivenessResult is null";

    size_t actualIn = 0, actualOut = 0;

    auto itIn = res->m_liveIn.find(blockId);
    if (itIn != res->m_liveIn.end())
        actualIn = itIn->second.size();

    auto itOut = res->m_liveOut.find(blockId);
    if (itOut != res->m_liveOut.end())
        actualOut = itOut->second.size();

    if (actualIn != expectedIn || actualOut != expectedOut)
    {
        return ::testing::AssertionFailure() << "Block " << blockId << " counts mismatch.\n"
                                             << "  Live-In Expected: " << expectedIn << ", Got: " << actualIn << "\n"
                                             << "  Live-Out Expected: " << expectedOut << ", Got: " << actualOut;
    }
    return ::testing::AssertionSuccess();
}

} // anonymous namespace

// ---------------------------------------------------------
// Core Analysis Tests
// ---------------------------------------------------------

TEST_F(LivenessAnalysisTest, TestStraightLineCode)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder opBuilder(ctx);
    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirInstructionBuilder builder(ctx, entryPoint, InsertionType::Append, entryPoint->end());

    MirRegister *v0 = createInt32Reg("v0");
    MirRegister *v1 = createInt32Reg("v1");
    MirInteger *imm10 = opBuilder.buildInt(getTypeTable()->i32(), FlexInt(10));

    // 1. MOV %v0, 10      -> DEF: %v0
    // 2. MOV %v1, %v0     -> USE: %v0, DEF: %v1
    // 3. RET %v1          -> USE: %v1
    builder.MOV(v0, imm10);
    builder.MOV(v1, v0);
    builder.RET(v1);

    getPassManager()->addPass<CodeFlowAnalysisPass>(ctx);
    getPassManager()->addPass<LivenessAnalysisPass>(ctx);
    LivenessResult *res = getPassManager()->getAnalysis<LivenessAnalysisPass>(ctx)->getResult();

    ASSERT_NE(res, nullptr);

    size_t bId = entryPoint->getId();

    // Local Verification
    EXPECT_TRUE(HasLocalDef(res, bId, v0->getRegId()));
    EXPECT_TRUE(HasLocalDef(res, bId, v1->getRegId()));

    // %v0 and %v1 are read AFTER being defined in the SAME block.
    // Therefore, they do NOT flow in from outside, so they are NOT in the local USE set.
    EXPECT_TRUE(NotLocalUse(res, bId, v0->getRegId()));
    EXPECT_TRUE(NotLocalUse(res, bId, v1->getRegId()));

    // Global Verification
    EXPECT_TRUE(HasLiveCounts(res, bId, 0, 0));
}

TEST_F(LivenessAnalysisTest, TestBranchingLiveness)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirOperandBuilder opBuilder(ctx);

    MirBlock *entryBlock = getTestFunc()->getEntryPoint();
    MirBlock *thenBlock = blockBuilder.build(nullptr, "then");
    MirBlock *elseBlock = blockBuilder.build(nullptr, "else");
    MirBlock *mergeBlock = blockBuilder.build(nullptr, "merge");

    MirRegister *v0 = createInt32Reg("v0");
    MirRegister *vCond = createInt1Reg("vCond");
    MirInteger *imm5 = opBuilder.buildInt(getTypeTable()->i32(), FlexInt(5));

    // Entry Block: Define %v0, evaluate conditional branch
    MirInstructionBuilder entryBuilder(ctx, entryBlock, InsertionType::Append, entryBlock->end());
    entryBuilder.MOV(v0, imm5);
    entryBuilder.CMP_EQ(vCond, v0, imm5);
    entryBuilder.BR_COND(vCond, opBuilder.buildRef(thenBlock), opBuilder.buildRef(elseBlock));

    // Then Block: Reads %v0
    MirInstructionBuilder thenBuilder(ctx, thenBlock, InsertionType::Append, thenBlock->end());
    thenBuilder.MOV(createInt32Reg("unused1"), v0);
    thenBuilder.JMP(opBuilder.buildRef(mergeBlock));

    // Else Block: Ignores %v0 entirely
    MirInstructionBuilder elseBuilder(ctx, elseBlock, InsertionType::Append, elseBlock->end());
    elseBuilder.MOV(createInt32Reg("unused2"), imm5);
    elseBuilder.JMP(opBuilder.buildRef(mergeBlock));

    // Merge Block: Clean exit
    MirInstructionBuilder mergeBuilder(ctx, mergeBlock, InsertionType::Append, mergeBlock->end());
    mergeBuilder.RET(imm5);

    getPassManager()->addPass<CodeFlowAnalysisPass>(ctx);
    getPassManager()->addPass<LivenessAnalysisPass>(ctx);
    LivenessResult *res = getPassManager()->getAnalysis<LivenessAnalysisPass>(ctx)->getResult();

    ASSERT_NE(res, nullptr);

    // %v0 MUST be live out of entry (needed by 'then')
    EXPECT_TRUE(IsLiveOut(res, entryBlock->getId(), v0->getRegId()));

    // %v0 MUST be live into 'then'
    EXPECT_TRUE(IsLiveIn(res, thenBlock->getId(), v0->getRegId()));

    // %v0 MUST NOT be live into 'else' (not read there or in merge)
    EXPECT_TRUE(NotLiveIn(res, elseBlock->getId(), v0->getRegId()));
}

TEST_F(LivenessAnalysisTest, TestInPlaceArithmetic)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder opBuilder(ctx);
    MirBlock *entryPoint = getTestFunc()->getEntryPoint();
    MirInstructionBuilder builder(ctx, entryPoint, InsertionType::Append, entryPoint->end());

    MirRegister *v0 = createInt32Reg("v0");
    MirInteger *imm1 = opBuilder.buildInt(getTypeTable()->i32(), FlexInt(1));

    // ADD is a 3-operand instruction: [0] Write, [1] Read, [2] Read.
    // By passing v0 as both the destination and the first source, we create a read-modify-write dependency.
    builder.ADD(v0, v0, imm1);

    getPassManager()->addPass<CodeFlowAnalysisPass>(ctx);
    getPassManager()->addPass<LivenessAnalysisPass>(ctx);
    LivenessResult *res = getPassManager()->getAnalysis<LivenessAnalysisPass>(ctx)->getResult();

    ASSERT_NE(res, nullptr);

    size_t bId = entryPoint->getId();

    // %v0 is both locally defined and locally used (read-modify-write)
    EXPECT_TRUE(HasLocalDef(res, bId, v0->getRegId()));
    EXPECT_TRUE(HasLocalUse(res, bId, v0->getRegId()));

    // Because it was used before a pure overwrite, it flows in from outside
    EXPECT_TRUE(IsLiveIn(res, bId, v0->getRegId()));
}

// ---------------------------------------------------------
// Bulletproof Edge Cases
// ---------------------------------------------------------

TEST_F(LivenessAnalysisTest, TestLoopLiveness)
{
    // Tests that liveness iteratively propagates UP a back-edge.
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirOperandBuilder opBuilder(ctx);

    MirBlock *entryBlock = getTestFunc()->getEntryPoint();
    MirBlock *headerBlock = blockBuilder.build(nullptr, "header");
    MirBlock *bodyBlock = blockBuilder.build(nullptr, "body");
    MirBlock *exitBlock = blockBuilder.build(nullptr, "exit");

    MirRegister *v0 = createInt32Reg("v0");
    MirRegister *v1 = createInt32Reg("v1");
    MirRegister *vCond = createInt1Reg("vCond");
    MirInteger *imm1 = opBuilder.buildInt(getTypeTable()->i32(), FlexInt(1));

    // Entry: %v0 = 1
    MirInstructionBuilder entryBuilder(ctx, entryBlock, InsertionType::Append, entryBlock->end());
    entryBuilder.MOV(v0, imm1);
    entryBuilder.JMP(opBuilder.buildRef(headerBlock));

    // Header: %v1 = %v0 + 1. If cond, jump Exit, else Body.
    MirInstructionBuilder headerBuilder(ctx, headerBlock, InsertionType::Append, headerBlock->end());
    headerBuilder.ADD(v1, v0, imm1);
    headerBuilder.CMP_EQ(vCond, v1, imm1);
    headerBuilder.BR_COND(vCond, opBuilder.buildRef(exitBlock), opBuilder.buildRef(bodyBlock));

    // Body: %v0 = %v1. Jump Header (back-edge)
    MirInstructionBuilder bodyBuilder(ctx, bodyBlock, InsertionType::Append, bodyBlock->end());
    bodyBuilder.MOV(v0, v1);
    bodyBuilder.JMP(opBuilder.buildRef(headerBlock));

    // Exit: RET %v1
    MirInstructionBuilder exitBuilder(ctx, exitBlock, InsertionType::Append, exitBlock->end());
    exitBuilder.RET(v1);

    getPassManager()->addPass<CodeFlowAnalysisPass>(ctx);
    getPassManager()->addPass<LivenessAnalysisPass>(ctx);
    LivenessResult *res = getPassManager()->getAnalysis<LivenessAnalysisPass>(ctx)->getResult();

    ASSERT_NE(res, nullptr);

    // %v0 must survive from Entry into the loop
    EXPECT_TRUE(IsLiveOut(res, entryBlock->getId(), v0->getRegId()));
    EXPECT_TRUE(IsLiveIn(res, headerBlock->getId(), v0->getRegId()));

    // %v1 is defined in Header, flows into Exit AND into Body
    EXPECT_TRUE(IsLiveOut(res, headerBlock->getId(), v1->getRegId()));
    EXPECT_TRUE(IsLiveIn(res, bodyBlock->getId(), v1->getRegId()));
    EXPECT_TRUE(IsLiveIn(res, exitBlock->getId(), v1->getRegId()));

    // %v0 is redefined in Body, and must flow up the back-edge to Header
    EXPECT_TRUE(IsLiveOut(res, bodyBlock->getId(), v0->getRegId()));
}

TEST_F(LivenessAnalysisTest, TestVariableRedefinitionKillsLiveness)
{
    // Tests that redefining a variable prevents its liveness from propagating further up.
    MirBuilderContext *ctx = getBuilderCtx();
    MirBlockBuilder blockBuilder(ctx, getTestFunc());
    MirOperandBuilder opBuilder(ctx);

    MirBlock *b1 = getTestFunc()->getEntryPoint();
    MirBlock *b2 = blockBuilder.build(nullptr, "b2");

    MirRegister *v0 = createInt32Reg("v0");
    MirRegister *v1 = createInt32Reg("v1");
    MirInteger *imm0 = opBuilder.buildInt(getTypeTable()->i32(), FlexInt(0));

    // B1: %v0 = 0
    MirInstructionBuilder b1Builder(ctx, b1, InsertionType::Append, b1->end());
    b1Builder.MOV(v0, imm0);
    b1Builder.JMP(opBuilder.buildRef(b2));

    // B2: Read %v0, then KILL %v0, then Read %v0 again
    MirInstructionBuilder b2Builder(ctx, b2, InsertionType::Append, b2->end());
    b2Builder.MOV(v1, v0);   // First read: uses B1's %v0
    b2Builder.MOV(v0, imm0); // Redefinition: KILLS the old %v0
    b2Builder.RET(v0);       // Second read: uses B2's internal %v0

    getPassManager()->addPass<CodeFlowAnalysisPass>(ctx);
    getPassManager()->addPass<LivenessAnalysisPass>(ctx);
    LivenessResult *res = getPassManager()->getAnalysis<LivenessAnalysisPass>(ctx)->getResult();

    ASSERT_NE(res, nullptr);

    // B2 uses %v0 BEFORE redefining it, so it is a local USE.
    EXPECT_TRUE(HasLocalUse(res, b2->getId(), v0->getRegId()));

    // B1 must push %v0 to B2
    EXPECT_TRUE(IsLiveOut(res, b1->getId(), v0->getRegId()));
    EXPECT_TRUE(IsLiveIn(res, b2->getId(), v0->getRegId()));
}

TEST_F(LivenessAnalysisTest, TestDeadCodeDefinitions)
{
    // Tests that a variable defined but never used doesn't falsely become live.
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder opBuilder(ctx);
    MirBlock *entryBlock = getTestFunc()->getEntryPoint();

    MirRegister *vAlive = createInt32Reg("vAlive");
    MirRegister *vDead = createInt32Reg("vDead");
    MirInteger *imm1 = opBuilder.buildInt(getTypeTable()->i32(), FlexInt(1));

    MirInstructionBuilder builder(ctx, entryBlock, InsertionType::Append, entryBlock->end());
    builder.MOV(vAlive, imm1);
    builder.MOV(vDead, imm1);
    builder.RET(vAlive);

    getPassManager()->addPass<CodeFlowAnalysisPass>(ctx);
    getPassManager()->addPass<LivenessAnalysisPass>(ctx);
    LivenessResult *res = getPassManager()->getAnalysis<LivenessAnalysisPass>(ctx)->getResult();

    ASSERT_NE(res, nullptr);

    // vDead is defined locally
    EXPECT_TRUE(HasLocalDef(res, entryBlock->getId(), vDead->getRegId()));

    // But it is NEVER live-in or live-out
    EXPECT_TRUE(NotLiveIn(res, entryBlock->getId(), vDead->getRegId()));
    EXPECT_TRUE(NotLiveOut(res, entryBlock->getId(), vDead->getRegId()));
}