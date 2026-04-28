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
    MirEmitter *emitter;
    MirFunction *func;
    MirPassManager *pm;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ctx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(ctx.get());
        
        // createFunction sets the created function as the current bound function in ctx.
        func = ctx->createFunction(1);
        
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
    MirBlock *b1 = ctx->createBlock();
    ctx->bindToBlock(b1);
    
    MirRegister r1 = emitter->createVirtualRegister(8); // To be def
    MirRegister r2 = emitter->createVirtualRegister(8); // To be def, then used
    
    emitter->emitMOV(MirOperand(r1), MirOperand(MirInteger{42})); // def r1
    emitter->emitMOV(MirOperand(r2), MirOperand(MirInteger{43})); // def r2
    emitter->emitADD(MirOperand(r1), MirOperand(r2));
    
    auto &la = pm->getAnalysis<LivenessAnalysis>(func, emitter);
    const LivenessResult &res = la.getResult();
    
    // Check local def/use
    auto itDef = res.m_def.find(b1);
    ASSERT_NE(itDef, res.m_def.end());
    
    // Check use
    auto itUse = res.m_use.find(b1);
    ASSERT_NE(itUse, res.m_use.end());
    // Since r2 IS defined before, it should NOT be in Use.
    EXPECT_EQ(itUse->second.size(), 0);
}

TEST_F(LivenessAnalysisTests, UseWithoutDef)
{
    MirBlock *b1 = ctx->createBlock();
    ctx->bindToBlock(b1);
    
    MirRegister r1 = emitter->createVirtualRegister(8); // Used without prior def in block
    MirRegister r2 = emitter->createVirtualRegister(8);
    
    emitter->emitMOV(MirOperand(r2), MirOperand(r1)); // read r1, write r2
    
    auto &la = pm->getAnalysis<LivenessAnalysis>(func, emitter);
    const LivenessResult &res = la.getResult();
    
    // r1 should be in use, r2 in def
    auto itUse = res.m_use.find(b1);
    ASSERT_NE(itUse, res.m_use.end());
    
    EXPECT_EQ(itUse->second.size(), 1);
    if (!itUse->second.empty()) {
        EXPECT_EQ((*itUse->second.begin()).m_id, r1.m_id);
    }
    
    auto itDef = res.m_def.find(b1);
    ASSERT_NE(itDef, res.m_def.end());
    EXPECT_EQ(itDef->second.size(), 1);
    if (!itDef->second.empty()) {
        EXPECT_EQ((*itDef->second.begin()).m_id, r2.m_id);
    }
}
