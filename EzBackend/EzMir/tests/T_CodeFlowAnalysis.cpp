/**
 * @file T_CodeFlowAnalysis.cpp
 * @brief Unit tests for CodeFlowAnalysis pass.
 */
#include <gtest/gtest.h>
#include <EzMir.h>
#include <MirPass/Passes/CodeFlowAnalysis.h>
#include <MirPass/MirPassManager.h>

class CodeFlowAnalysisTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> ctx;
    MirEmitter *emitter;
    MirFunction *func;
    MirPassManager *pm;
    CodeFlowAnalysis *pass;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ctx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(ctx.get());

        func = ctx->createFunction(ctx->createType(MirTypeKind::Void, nullptr, "myType")->getId());
        ctx->bindToBlock(*func->getBlocks()->begin());

        pm = new MirPassManager();
        pass = new CodeFlowAnalysis(emitter);

        ec->beginScope();
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete pass;
        delete pm;
        delete emitter;
    }
};

TEST_F(CodeFlowAnalysisTests, EmptyFunctionFails)
{
    EXPECT_TRUE(pass->run(func, pm));
}

TEST_F(CodeFlowAnalysisTests, SingleBlock)
{
    MirBlock *b1 = ctx->createBlock();
    ctx->bindToBlock(b1);
    emitter->emitNOP();

    EXPECT_TRUE(pass->run(func, pm));

    const ControlFlowResult &res = pass->getResult();
    EXPECT_TRUE(res.m_successors.empty());
    EXPECT_TRUE(res.m_predecessors.empty());
}

TEST_F(CodeFlowAnalysisTests, UnconditionalJump)
{
    MirBlock *b1 = ctx->createBlock();
    MirBlock *b2 = ctx->createBlock();

    ctx->bindToBlock(b1);
    emitter->emitJMP(ctx->createReference(b2));

    ctx->bindToBlock(b2);
    emitter->emitNOP();

    EXPECT_TRUE(pass->run(func, pm));
}

TEST_F(CodeFlowAnalysisTests, MultipleBlocksFallthrough)
{
    MirBlock *b1 = ctx->createBlock();
    MirBlock *b2 = ctx->createBlock();

    ctx->bindToBlock(b1);
    emitter->emitNOP(); // Implicit fallthrough to b2

    ctx->bindToBlock(b2);
    emitter->emitNOP();

    EXPECT_TRUE(pass->run(func, pm));
}

TEST_F(CodeFlowAnalysisTests, ReturnInstruction)
{
    MirBlock *b1 = ctx->createBlock();
    ctx->bindToBlock(b1);

    MirRegister r = emitter->createVirtualRegister(8);
    emitter->emitRET(MirOperand(r));

    EXPECT_TRUE(pass->run(func, pm));
}
