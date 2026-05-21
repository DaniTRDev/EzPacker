/**
 * @file T_PromoteTypeLegalizer.cpp
 * @brief Unit tests for PromoteTypeLegalizer.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetLegalizer/StandardLegalizers/PromoteTypeLegalizer.h>
#include <filesystem>

class DummyABIDesc : public ABIDesc
{
  public:
    DummyABIDesc()
    {
        setRegSizeInBits(64); // 8 bytes
    }

    ArgLocation getArgLoc(size_t id, MirType *type) const override { return ArgLocation(); }

    const char *getName() const override { return "DummyAbi"; }

    /**
     * Returns the strict ABI alignment required for the given type.
     * This is used for struct packing, stack frames, and array layouts.
     */
    size_t getAbiAlignment(MirType *type) const
    {
        if (!type)
            return 1;

        // Handle Basic Types (Integers, Floats)
        size_t size = type->getTotalSizeInBytes();

        // Standard rule: basic types align to their own size, capped by the target max.
        // E.g., size 4 aligns to 4. Size 8 aligns to 8 (or 4 on 32-bit systems).
        size_t align = size;

        // Ensure it's a power of 2 (rounds up sizes like 3 to 4)
        align = std::bit_ceil(align); // C++20 feature, or write a quick power-of-2 helper
        return align;
    }

    /**
     * Returns the preferred alignment for the given type.
     * Used for global variables to optimize CPU cache line fetching.
     */
    size_t getPreferredAlignment(MirType *type) const { return getAbiAlignment(type); }
};

class PromoteTypeLegalizerTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> ctx;
    MirEmitter *emitter;
    DummyABIDesc abi;
    TargetDesc *targetDesc;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ctx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(ctx.get());

        targetDesc = new TargetDesc(&abi, TargetEndianness::LittleEndian, "DummyTarget");

        ec->beginScope();
        MirBlock *block = ctx->createBlock();
        ctx->bindToBlock(block);
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete emitter;
        delete targetDesc;
    }
};

TEST_F(PromoteTypeLegalizerTests, IgnoreLegalSizedRegister)
{
    MirRegister *r = emitter->createVirtualRegister(emitter->getContext()->getIntegerTypeBySize(8)); // 8 bytes
    MirInstruction *instr = emitter->emitMOV(r, r);

    auto it = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    auto opIt = instr->getOperands()->begin();

    bool modified = StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, opIt, 0);
    EXPECT_FALSE(modified);
}

TEST_F(PromoteTypeLegalizerTests, PromoteOutputRegister)
{
    MirRegister *r = emitter->createVirtualRegister(
            emitter->getContext()->getIntegerTypeBySize(4)); // 4 bytes, smaller than 8 bytes
    MirInstruction *instr = emitter->emitMOV(r, r);

    auto it = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    auto opIt = instr->getOperands()->begin(); // Destination operand (Write)

    bool modified = StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, opIt, 0);
    EXPECT_TRUE(modified);
    EXPECT_EQ((*opIt)->get<MirRegister>()->getSizeInBytes(), 8u);

    // We expect a TRUNC instruction emitted AFTER this one.
    // So the list should be MOV, TRUNC.
    auto nextIt = it;
    ++nextIt;
    EXPECT_NE(nextIt, ctx->getCurrentBoundBlock()->getInstructions()->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::TRUNC);
}

TEST_F(PromoteTypeLegalizerTests, PromoteInputRegister)
{
    MirRegister *r = emitter->createVirtualRegister(
            emitter->getContext()->getIntegerTypeBySize(4)); // 4 bytes, smaller than 8 bytes
    MirInstruction *instr = emitter->emitMOV(r, r);

    auto it = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    auto opIt = instr->getOperands()->begin();
    ++opIt; // Source operand (Input)

    bool modified = StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, opIt, 1);
    EXPECT_TRUE(modified);
    EXPECT_EQ((*opIt)->get<MirRegister>()->getSizeInBytes(), 8u);

    // We expect an EXT (SEXT or ZEXT) instruction emitted BEFORE this one.
    // So the list should be ZEXT/SEXT, MOV.
    auto firstIt = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    EXPECT_NE((*firstIt)->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_TRUE((*firstIt)->getOpCode() == MirInstructionOpCode::ZEXT ||
                (*firstIt)->getOpCode() == MirInstructionOpCode::SEXT);
}

TEST_F(PromoteTypeLegalizerTests, PromoteImmediate)
{
    MirRegister *r = emitter->createVirtualRegister(emitter->getContext()->getIntegerTypeBySize(8));
    MirInteger *i1 = emitter->createImmediateInteger(emitter->getContext()->getIntegerTypeBySize(4), 42);

    MirInstruction *instr = emitter->emitMOV(r, i1); // 4-byte int

    auto it = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    auto opIt = instr->getOperands()->begin();
    ++opIt; // Source operand

    bool modified = StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, opIt, 1);
    EXPECT_TRUE(modified);
    EXPECT_EQ((*opIt)->get<MirInteger>()->getSizeInBytes(), 8u);
    EXPECT_EQ((*opIt)->get<MirInteger>()->getValue(), 42);
}
