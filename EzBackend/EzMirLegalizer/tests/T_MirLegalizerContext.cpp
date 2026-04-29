/**
 * @file T_MirLegalizerContext.cpp
 * @brief Unit tests for MirLegalizerContext.
 */
#include <gtest/gtest.h>
#include <MirLegalizerContext.h>
#include <ABIDesc.h>
#include <EzMir.h>
#include <memory>

class Mips64Abi : public ABIDesc
{
  public:
    const ArgLocation &getArgLoc(size_t id) const override
    {
        // Not needed for the test.
        static ArgLocation test{};
        return test;
    }
};

class MirLegalizerContextTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;
    MirPassManager *passManager;
    Mips64Abi abi;
    std::shared_ptr<MirLegalizerContext> ctx;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());
        passManager = new MirPassManager();
        ctx = std::make_shared<MirLegalizerContext>(ec, sm);

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
    ctx->setEmitter(emitter);
    EXPECT_EQ(ctx->getEmitter(), emitter);
}

TEST_F(MirLegalizerContextTests, SetAndGetAbiDesc)
{
    ctx->setAbiDesc(&abi);
    EXPECT_EQ(ctx->getAbiDesc(), &abi);
}

TEST_F(MirLegalizerContextTests, SetAndGetPassManager)
{
    ctx->setPassManager(passManager);
    EXPECT_EQ(ctx->getPassManager(), passManager);
}
