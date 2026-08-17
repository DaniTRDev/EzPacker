#include "EzMirTestSuite.h"
#include "Class/MirClass.h"
#include "Class/MirClassBuilder.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "MirPasses/Passes/ClassOffsetResolverPass.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

class TestClassOffsetResolver : public MirTestSuiteAsGtest
{
  public:
};

namespace
{
::testing::AssertionResult HasClassLayout(MirClass *cls, size_t expectedSize, size_t expectedAlignment)
{
    if (!cls)
        return ::testing::AssertionFailure() << "Class is nullptr";
    if (!cls->getType())
        return ::testing::AssertionFailure() << "Class type is nullptr";

    size_t actualSize = cls->getType()->getTotalSizeInBytes();
    size_t actualAlign = cls->getType()->getMaxAlignmentInBytes();

    if (actualSize != expectedSize)
        return ::testing::AssertionFailure()
                << "Class size expected " << expectedSize << " bytes, but got " << actualSize;

    if (actualAlign != expectedAlignment)
        return ::testing::AssertionFailure()
                << "Class alignment expected " << expectedAlignment << " bytes, but got " << actualAlign;

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult HasFieldOffset(MirClass *cls, size_t index, size_t expectedOffset)
{
    if (!cls)
        return ::testing::AssertionFailure() << "Class is nullptr";

    auto field = cls->getFieldById(index);
    if (!field)
        return ::testing::AssertionFailure() << "Field at index " << index << " not found";

    if (field->m_offset != expectedOffset)
        return ::testing::AssertionFailure() << "Field " << index << " ('" << field->m_name << "') expected offset "
                                             << expectedOffset << ", but got " << field->m_offset;

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult HasMethodOffset(MirClass *cls, size_t index, size_t expectedOffset)
{
    if (!cls)
        return ::testing::AssertionFailure() << "Class is nullptr";

    auto method = cls->getMethodById(index);
    if (!method)
        return ::testing::AssertionFailure() << "Method at index " << index << " not found";

    if (method->m_offset != expectedOffset)
        return ::testing::AssertionFailure()
                << "Method " << index << " expected offset " << expectedOffset << ", but got " << method->m_offset;

    return ::testing::AssertionSuccess();
}

} // anonymous namespace

TEST_F(TestClassOffsetResolver, TestBaseClassSimpleLayoutResolution)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirClassBuilder builder(ctx);
    builder.appendField(types->i32(), "m_id");                    // 4 bytes size, 4-byte align
    builder.appendField(types->getPtr(types->i8()), "m_namePtr"); // 8 bytes size, 8-byte align

    MirClass *baseClass = builder.build(nullptr, "BaseClass");

    // Run our new layout compilation pass on the context state
    ClassOffsetResolverPass *pass = runPass<ClassOffsetResolverPass>(ctx);

    // Verification details:
    // Field m_id starts at 0 (size 4). Next available is 4.
    // m_namePtr requires 8-byte alignment, forcing 4 bytes of padding. It starts at offset 8 (size 8).
    // Total footprint size = 8 + 8 = 16 bytes. Max alignment seen is 8 bytes.
    EXPECT_TRUE(HasClassLayout(baseClass, 16, 8));
    EXPECT_TRUE(HasFieldOffset(baseClass, 0, 0));
    EXPECT_TRUE(HasFieldOffset(baseClass, 1, 8));
}

TEST_F(TestClassOffsetResolver, TestVirtualClassLayoutWithVTableSlot)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();
    MirFunctionBuilder funcBuilder(ctx);

    MirFunction *mockFunc = funcBuilder.build(types->getVoidType(), "virtualFunc");

    MirClassBuilder builder(ctx);
    builder.appendField(types->i64(), "m_data"); // 8 bytes size, 8-byte align
    builder.appendMethod(mockFunc);

    MirClass *vClass = builder.build(nullptr, "VirtualClass");

    ClassOffsetResolverPass *pass = runPass<ClassOffsetResolverPass>(ctx);

    // Verification details:
    // "vTable" pointer automatically added at index 0 (size 8, offset 0).
    // m_data starts at next aligned position: offset 8 (size 8).
    // Total size = 16 bytes. Max alignment = 8. VTable slot 0 matches function offset 0.
    EXPECT_TRUE(HasClassLayout(vClass, 16, 8));
    EXPECT_TRUE(HasFieldOffset(vClass, 0, 0));
    EXPECT_TRUE(HasFieldOffset(vClass, 1, 8));
    EXPECT_TRUE(HasMethodOffset(vClass, 0, 0));
}

TEST_F(TestClassOffsetResolver, TestSingleInheritanceLayoutResolution)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();
    MirFunctionBuilder funcBuilder(ctx);

    // Create the parent context structures
    MirFunction *baseFoo = funcBuilder.build(types->getVoidType(), "foo");
    MirFunction *baseBar = funcBuilder.build(types->i32(), "bar");

    MirClassBuilder baseBuilder(ctx);
    baseBuilder.appendField(types->i32(), "m_baseVal");
    baseBuilder.appendMethod(baseFoo);
    baseBuilder.appendMethod(baseBar);
    MirClass *parentClass = baseBuilder.build(nullptr, "Parent");

    // Create the child context structures
    MirFunction *derivedBarOverride = funcBuilder.build(types->i32(), "bar");
    MirFunction *derivedNewFunc = funcBuilder.build(types->getVoidType(), "baz");

    MirClassBuilder childBuilder(ctx);
    childBuilder.appendField(types->f64(), "m_childVal");
    childBuilder.appendMethod(derivedBarOverride);
    childBuilder.appendMethod(derivedNewFunc);
    MirClass *childClass = childBuilder.build(parentClass, "Child");

    // Execute the layout compiler pass
    ClassOffsetResolverPass *pass = runPass<ClassOffsetResolverPass>(ctx);

    // Parent Layout check:
    // vTable ptr (0, size 8) -> m_baseVal (8, size 4) -> tail-padded to multiple of 8 = 16 bytes.
    EXPECT_TRUE(HasClassLayout(parentClass, 16, 8));
    EXPECT_TRUE(HasFieldOffset(parentClass, 0, 0)); // vtable
    EXPECT_TRUE(HasFieldOffset(parentClass, 1, 8)); // m_baseVal
    EXPECT_TRUE(HasMethodOffset(parentClass, 0, 0));
    EXPECT_TRUE(HasMethodOffset(parentClass, 1, 8));

    // Child Layout check:
    // Inherits base size 16. m_childVal (f64, size 8, align 8) can fit directly at offset 16.
    // Total child footprint size = 16 + 8 = 24 bytes.
    EXPECT_TRUE(HasClassLayout(childClass, 24, 8));
    EXPECT_TRUE(HasFieldOffset(childClass, 0, 0));  // inherited vtable
    EXPECT_TRUE(HasFieldOffset(childClass, 1, 8));  // inherited m_baseVal
    EXPECT_TRUE(HasFieldOffset(childClass, 2, 16)); // m_childVal

    // VTable total slots = 3. Slot 0 (foo: 0), Slot 1 (bar override: 8), Slot 2 (baz new: 16).
    EXPECT_TRUE(HasMethodOffset(childClass, 0, 0));
    EXPECT_TRUE(HasMethodOffset(childClass, 1, 8));
    EXPECT_TRUE(HasMethodOffset(childClass, 2, 16));
}

TEST_F(TestClassOffsetResolver, TestClassPaddingAndAlignment)
{
    // Tests that internal field padding and tail padding are correctly calculated
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirClassBuilder builder(ctx);
    builder.appendField(types->i8(), "m_a");  // size 1, align 1
    builder.appendField(types->i64(), "m_b"); // size 8, align 8 -> needs 7 bytes of padding before it
    builder.appendField(types->i16(), "m_c"); // size 2, align 2
    builder.appendField(types->i32(), "m_d"); // size 4, align 4 -> needs 2 bytes of padding before it

    MirClass *packedClass = builder.build(nullptr, "PackedClass");

    runPass<ClassOffsetResolverPass>(ctx);

    // Layout math:
    // m_a: offset 0 (ends at 1)
    // padding: 7 bytes (to reach align 8)
    // m_b: offset 8 (ends at 16)
    // m_c: offset 16 (ends at 18)
    // padding: 2 bytes (to reach align 4)
    // m_d: offset 20 (ends at 24)
    // Total size = 24. Max align = 8. Tail padding fits naturally.
    EXPECT_TRUE(HasClassLayout(packedClass, 24, 8));

    EXPECT_TRUE(HasFieldOffset(packedClass, 0, 0));  // m_a
    EXPECT_TRUE(HasFieldOffset(packedClass, 1, 8));  // m_b
    EXPECT_TRUE(HasFieldOffset(packedClass, 2, 16)); // m_c
    EXPECT_TRUE(HasFieldOffset(packedClass, 3, 20)); // m_d
}

TEST_F(TestClassOffsetResolver, TestMultiLevelInheritance)
{
    // Tests deep inheritance hierarchy (Grandparent -> Parent -> Child)
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    // 1. Grandparent (Base)
    MirClassBuilder grandBuilder(ctx);
    grandBuilder.appendField(types->i32(), "m_baseVal");
    MirClass *grandParentClass = grandBuilder.build(nullptr, "GrandParent");

    // 2. Parent (Mid)
    MirClassBuilder parentBuilder(ctx);
    parentBuilder.appendField(types->i64(), "m_midVal");
    MirClass *parentClass = parentBuilder.build(grandParentClass, "Parent");

    // 3. Child (Leaf)
    MirClassBuilder childBuilder(ctx);
    childBuilder.appendField(types->i8(), "m_leafVal");
    MirClass *childClass = childBuilder.build(parentClass, "Child");

    runPass<ClassOffsetResolverPass>(ctx);

    // Grandparent Layout: m_baseVal (offset 0, size 4). Size: 4, Align: 4.
    EXPECT_TRUE(HasClassLayout(grandParentClass, 4, 4));
    EXPECT_TRUE(HasFieldOffset(grandParentClass, 0, 0));

    // Parent Layout: Inherits 4 bytes. m_midVal needs 8-byte align, padding to offset 8.
    // Total Size: 16 (8 + 8), Max Align: 8.
    EXPECT_TRUE(HasClassLayout(parentClass, 16, 8));
    EXPECT_TRUE(HasFieldOffset(parentClass, 0, 0)); // m_baseVal (inherited)
    EXPECT_TRUE(HasFieldOffset(parentClass, 1, 8)); // m_midVal

    // Child Layout: Inherits 16 bytes. m_leafVal fits at 16 (1-byte align).
    // Tail padded to max align 8. Total Size: 24.
    EXPECT_TRUE(HasClassLayout(childClass, 24, 8));
    EXPECT_TRUE(HasFieldOffset(childClass, 0, 0));  // m_baseVal (inherited)
    EXPECT_TRUE(HasFieldOffset(childClass, 1, 8));  // m_midVal (inherited)
    EXPECT_TRUE(HasFieldOffset(childClass, 2, 16)); // m_leafVal
}