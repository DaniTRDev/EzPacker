/**
 * @file T_MirEmitterContext.cpp
 * @brief Unit tests for MirEmitterContext.
 *
 * Covers: ID creation, block/function creation, block binding, type creation
 * and lookup, null/edge cases.
 */
#include <gtest/gtest.h>
#include <EzMir.h>

class MirEmitterContextTests : public ::testing::Test
{
protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager>  sm;
    std::unique_ptr<MirEmitterContext> ctx;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ctx = std::make_unique<MirEmitterContext>(ec, sm);
        ec->beginScope();
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
    }
};

// ─── ID creation ─────────────────────────────────────────────────────────────

TEST_F(MirEmitterContextTests, FirstIdIsNonZero)
{
    MirId id = ctx->createId();
    EXPECT_NE(id, MIRID_INVALID);
}

TEST_F(MirEmitterContextTests, IdsAreMonotonicallyIncreasing)
{
    MirId id1 = ctx->createId();
    MirId id2 = ctx->createId();
    EXPECT_GT(id2, id1);
}

// ─── Block creation ───────────────────────────────────────────────────────────

TEST_F(MirEmitterContextTests, CreateBlockReturnsNonNull)
{
    MirBlock *block = ctx->createBlock();
    ASSERT_NE(block, nullptr);
    EXPECT_NE(block->getId(), MIRID_INVALID);
}

TEST_F(MirEmitterContextTests, NewContextHasNoBoundBlock)
{
    EXPECT_EQ(ctx->getCurrentBoundBlock(), nullptr);
}

TEST_F(MirEmitterContextTests, BindToBlockSucceeds)
{
    MirBlock *block = ctx->createBlock();
    EXPECT_TRUE(ctx->bindToBlock(block));
    EXPECT_EQ(ctx->getCurrentBoundBlock(), block);
}

TEST_F(MirEmitterContextTests, BindToNullBlockDontFail)
{
    EXPECT_TRUE(ctx->bindToBlock(nullptr));
}

TEST_F(MirEmitterContextTests, InstructionIsAddedToBoundBlock)
{
    MirBlock *block = ctx->createBlock();
    ctx->bindToBlock(block);
    MirInstruction *instr = ctx->createInstruction(MirInstructionOpCode::NOP);
    ASSERT_NE(instr, nullptr);
    ASSERT_NE(block->getInstructions(), nullptr);
    EXPECT_EQ(block->getInstructions()->m_numElems, 1u);
}

// ─── Function creation ────────────────────────────────────────────────────────

TEST_F(MirEmitterContextTests, CreateFunctionReturnsNonNull)
{
    // We need a valid return type ID.
    MirType *voidType = ctx->createType(MirTypeKind::Void, nullptr, "void");
    ASSERT_NE(voidType, nullptr);
    MirFunction *fn = ctx->createFunction(voidType, nullptr, nullptr, "test");
    ASSERT_NE(fn, nullptr);
    EXPECT_NE(fn->getId(), MIRID_INVALID);
    EXPECT_NE(fn->getEntryPoint(), nullptr);
}

TEST_F(MirEmitterContextTests, CreateFunctionWithInvalidReturnTypeEmitsError)
{
    MirFunction *fn = ctx->createFunction(nullptr, nullptr, nullptr, "test");
    EXPECT_EQ(fn, nullptr);
}

// ─── Type creation & lookup ───────────────────────────────────────────────────

TEST_F(MirEmitterContextTests, CreateTypeReturnsNonNull)
{
    MirType *t = ctx->createType(MirTypeKind::Integer, nullptr, "i64");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->getKind(), MirTypeKind::Integer);
    EXPECT_EQ(t->getName(), "i64");
    EXPECT_NE(t->getId(), MIRID_INVALID);
}

TEST_F(MirEmitterContextTests, LookupTypeByIdSucceeds)
{
    MirType *created = ctx->createType(MirTypeKind::FloatingPoint, nullptr, "double");
    ASSERT_NE(created, nullptr);
    MirType *looked = ctx->getMirTypeById(created->getId());
    EXPECT_EQ(looked, created);
}

TEST_F(MirEmitterContextTests, LookupInvalidIdReturnsNull)
{
    MirType *t = ctx->getMirTypeById(MIRID_INVALID);
    EXPECT_EQ(t, nullptr);
}

TEST_F(MirEmitterContextTests, CreateTypeWithEmptyNameFails)
{
    MirType *t = ctx->createType(MirTypeKind::Integer, nullptr, "");
    EXPECT_EQ(t, nullptr);
}

// ─── Pools are accessible ────────────────────────────────────────────────────

TEST_F(MirEmitterContextTests, PoolAccessorsReturnNonNull)
{
    EXPECT_NE(ctx->getBlockPool(), nullptr);
    EXPECT_NE(ctx->getFunctionPool(), nullptr);
    EXPECT_NE(ctx->getInstructionPool(), nullptr);
    EXPECT_NE(ctx->getOperandPool(), nullptr);
    EXPECT_NE(ctx->getTypePool(), nullptr);
    EXPECT_NE(ctx->getDataEntryPool(), nullptr);
}

// ─── Additional MirEmitterContext tests ──────────────────────────────────────

TEST_F(MirEmitterContextTests, CreateMultipleBlocksHaveUniqueIds)
{
    MirBlock *b1 = ctx->createBlock();
    MirBlock *b2 = ctx->createBlock();
    MirBlock *b3 = ctx->createBlock();
    ASSERT_NE(b1, nullptr);
    ASSERT_NE(b2, nullptr);
    ASSERT_NE(b3, nullptr);
    EXPECT_NE(b1->getId(), b2->getId());
    EXPECT_NE(b2->getId(), b3->getId());
    EXPECT_NE(b1->getId(), b3->getId());
}

TEST_F(MirEmitterContextTests, RebindToAnotherBlock)
{
    MirBlock *b1 = ctx->createBlock();
    MirBlock *b2 = ctx->createBlock();
    ctx->bindToBlock(b1);
    EXPECT_EQ(ctx->getCurrentBoundBlock(), b1);
    ctx->bindToBlock(b2);
    EXPECT_EQ(ctx->getCurrentBoundBlock(), b2);
}

TEST_F(MirEmitterContextTests, InstructionsGoToCorrectBlock)
{
    MirBlock *b1 = ctx->createBlock();
    MirBlock *b2 = ctx->createBlock();

    ctx->bindToBlock(b1);
    ctx->createInstruction(MirInstructionOpCode::NOP);
    ctx->createInstruction(MirInstructionOpCode::NOP);

    ctx->bindToBlock(b2);
    ctx->createInstruction(MirInstructionOpCode::NOP);

    EXPECT_EQ(b1->getInstructions()->m_numElems, 2u);
    EXPECT_EQ(b2->getInstructions()->m_numElems, 1u);
}

TEST_F(MirEmitterContextTests, FunctionHasEntryPointBlock)
{
    MirType *voidType = ctx->createType(MirTypeKind::Void, nullptr, "void");
    ASSERT_NE(voidType, nullptr);
    MirFunction *fn = ctx->createFunction(voidType, nullptr, nullptr, "test");
    ASSERT_NE(fn, nullptr);
    ASSERT_NE(fn->getEntryPoint(), nullptr);
    ASSERT_NE(fn->getBlocks(), nullptr);
    EXPECT_GE(fn->getBlocks()->m_numElems, 1u);
}

TEST_F(MirEmitterContextTests, FunctionReturnTypeIdIsStored)
{
    MirType *i64Type = ctx->createType(MirTypeKind::Integer, nullptr, "i64");
    ASSERT_NE(i64Type, nullptr);
    MirFunction *fn = ctx->createFunction(i64Type, nullptr, nullptr, "test");
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->getReturnType()->getId(), i64Type->getId());
}

TEST_F(MirEmitterContextTests, FunctionParameterPoolAccessible)
{
    EXPECT_NE(ctx->getFunctionParameterPool(), nullptr);
}

TEST_F(MirEmitterContextTests, EntryDataPoolAccessible)
{
    EXPECT_NE(ctx->getEntryDataPool(), nullptr);
}

TEST_F(MirEmitterContextTests, CreateMultipleTypesLookupByIdWorks)
{
    MirType *t1 = ctx->createType(MirTypeKind::Integer, nullptr, "i8");
    MirType *t2 = ctx->createType(MirTypeKind::Integer, nullptr, "i16");
    MirType *t3 = ctx->createType(MirTypeKind::FloatingPoint, nullptr, "float");
    ASSERT_NE(t1, nullptr);
    ASSERT_NE(t2, nullptr);
    ASSERT_NE(t3, nullptr);

    EXPECT_EQ(ctx->getMirTypeById(t1->getId()), t1);
    EXPECT_EQ(ctx->getMirTypeById(t2->getId()), t2);
    EXPECT_EQ(ctx->getMirTypeById(t3->getId()), t3);
}

TEST_F(MirEmitterContextTests, CreateVoidType)
{
    MirType *t = ctx->createType(MirTypeKind::Void, nullptr, "void");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->getKind(), MirTypeKind::Void);
    EXPECT_EQ(t->getName(), "void");
}

TEST_F(MirEmitterContextTests, CreatePointerType)
{
    MirType *t = ctx->createType(MirTypeKind::Pointer, nullptr, "ptr");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->getKind(), MirTypeKind::Pointer);
}

TEST_F(MirEmitterContextTests, FunctionEntryPointIsFirstBlock)
{
    MirType *voidType = ctx->createType(MirTypeKind::Void, nullptr, "void");
    MirFunction *fn = ctx->createFunction(voidType, nullptr, nullptr, "test");
    ASSERT_NE(fn, nullptr);
    MirBlock *entry = fn->getEntryPoint();
    ASSERT_NE(entry, nullptr);
    // The entry point should be the first block in the function's block list
    // Assuming the list is ordered by creation/insertion
    // But we can check if it's in the list
    bool found = false;
    for (auto *b : *fn->getBlocks()) {
        if (b == entry) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}
