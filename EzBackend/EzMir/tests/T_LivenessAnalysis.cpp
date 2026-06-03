/**
 * @file T_LivenessAnalysis.cpp
 * @brief Unit tests for LivenessAnalysis pass.
 */
#include <gtest/gtest.h>
#include <EzMir.h>
#include <MirPass/Passes/LivenessAnalysis.h>
#include <MirPass/Passes/CodeFlowAnalysis.h>

class LivenessAnalysisTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> ctx;
    std::shared_ptr<MirTypeTable> m_types;
    MirEmitter *emitter;
    MirFunction *func;
    MirPassManager *pm;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ctx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(ctx.get());
        m_types = std::make_shared<MirTypeTable>();

        // createFunction sets the created function as the current bound function in ctx.
        m_types->initialize(ctx.get());
        func = ctx->createFunction(m_types->getVoidType(), nullptr, "test");

        pm = new MirPassManager();

        ec->beginScope();
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete pm;
        delete emitter;
    }
};

TEST_F(LivenessAnalysisTests, DefAndUseSets)
{
    MirBlock *b1 = func->getBlocks()->get<MirBlock>(0);
    ctx->setInsertPoint(b1);

    MirRegister *r1 = emitter->createVirtualRegister(m_types->getInt64Type()); // To be def
    MirRegister *r2 = emitter->createVirtualRegister(m_types->getInt64Type()); // To be def, then used

    MirInteger *i1 = emitter->createImmediateInteger(m_types->getInt64Type(), 42);
    MirInteger *i2 = emitter->createImmediateInteger(m_types->getInt64Type(), 43);

    emitter->emitMOV(r1, i1); // def r1
    emitter->emitMOV(r2, i2); // def r2
    emitter->emitADD(r1, r2);

    auto &la = pm->getAnalysis<LivenessAnalysis>(func->getBlocks(), func->getBlocks()->begin(), emitter);
    const LivenessResult &res = la.getResult();

    // Check local def/use
    auto itDef = res.m_def.find(b1->getId());
    ASSERT_NE(itDef, res.m_def.end());

    // Check use
    auto itUse = res.m_use.find(b1->getId());
    ASSERT_NE(itUse, res.m_use.end());
    // Since r2 IS defined before, it should NOT be in Use.
    EXPECT_EQ(itUse->second.size(), 0);
}

TEST_F(LivenessAnalysisTests, UseWithoutDef)
{
    MirBlock *b1 = func->getBlocks()->get<MirBlock>(0);
    ctx->setInsertPoint(b1);

    MirRegister *r1 = emitter->createVirtualRegister(m_types->getInt64Type()); // Used without prior def in block
    MirRegister *r2 = emitter->createVirtualRegister(m_types->getInt64Type());

    emitter->emitMOV(r2, r1); // write r2, read r1

    auto &la = pm->getAnalysis<LivenessAnalysis>(func->getBlocks(), func->getBlocks()->begin(), emitter);
    const LivenessResult &res = la.getResult();

    // r1 should be in use, r2 in def
    auto itUse = res.m_use.find(b1->getId());
    ASSERT_NE(itUse, res.m_use.end());

    EXPECT_EQ(itUse->second.size(), 1);
    if (!itUse->second.empty())
    {
        EXPECT_EQ((*itUse->second.begin())->getRegId(), r1->getRegId());
    }

    auto itDef = res.m_def.find(b1->getId());
    ASSERT_NE(itDef, res.m_def.end());
    EXPECT_EQ(itDef->second.size(), 1);
    if (!itDef->second.empty())
    {
        EXPECT_EQ((*itDef->second.begin())->getRegId(), r2->getRegId());
    }
}
