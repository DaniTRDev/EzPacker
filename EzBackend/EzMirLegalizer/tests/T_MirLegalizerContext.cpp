/**
 * @file T_MirLegalizerContext.cpp
 * @brief Unit tests for MirLegalizerContext.
 */
#include <gtest/gtest.h>
#include <MirLegalizerContext.h>
#include <ABIDesc.h>
#include <EzMir.h>
#include <memory>

class MirLegalizerContextTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter* emitter;
    MirPassManager* passManager;
    ABIDesc abi;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());
        passManager = new MirPassManager();

        abi.setRegSizeInBits(32);
        abi.setEndianness(LittleEndian);
    }

    void TearDown() override
    {
        delete emitter;
        delete passManager;
    }
};

TEST_F(MirLegalizerContextTests, SetAndGetEmitter)
{
    MirLegalizerContext ctx;
    ctx.setEmitter(emitter);
    EXPECT_EQ(ctx.getEmitter(), emitter);
}

TEST_F(MirLegalizerContextTests, SetAndGetAbiDesc)
{
    MirLegalizerContext ctx;
    ctx.setAbiDesc(&abi);
    EXPECT_EQ(ctx.getAbiDesc(), &abi);
}

TEST_F(MirLegalizerContextTests, SetAndGetPassManager)
{
    MirLegalizerContext ctx;
    ctx.setPassManager(passManager);
    EXPECT_EQ(ctx.getPassManager(), passManager);
}
