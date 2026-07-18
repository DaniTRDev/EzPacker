#include <gtest/gtest.h>
#include "EzMirTestSuite/EzMirTestSuite.h"

class TestClassOffsetResolver : public MirTestSuiteAsGtest
{
  public:
};

TEST_F(TestClassOffsetResolver, TestBaseClassSimpleLayoutResolution)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirClassBuilder builder(ctx);
    builder.appendField(types->i32(), "m_id");                    // 4 bytes size, 4-byte align
    builder.appendField(types->getPtr(types->i8()), "m_namePtr"); // 8 bytes size, 8-byte align

    MirClass *baseClass = builder.build(nullptr, "BaseClass");
    size_t classId = baseClass->getId();

    // Run our new layout compilation pass on the context state
    ClassOffsetResolver *pass = runPass<ClassOffsetResolver>(ctx);
    ClassOffsetResolverVerifier verifier(pass, ctx);

    verifier.executed().succeeded().mirModified();

    // Verification details:
    // Field m_id starts at 0 (size 4). Next available is 4.
    // m_namePtr requires 8-byte alignment, forcing 4 bytes of padding. It starts at offset 8 (size 8).
    // Total footprint size = 8 + 8 = 16 bytes. Max alignment seen is 8 bytes.
    verifier.classSize(classId, 16)
            .classAlignment(classId, 8)
            .fieldOffset(classId, "m_id", 0)
            .fieldOffset(classId, "m_namePtr", 8);
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
    size_t classId = vClass->getId();

    ClassOffsetResolver *pass = runPass<ClassOffsetResolver>(ctx);
    ClassOffsetResolverVerifier verifier(pass, ctx);

    verifier.executed().succeeded().mirModified();

    // Verification details:
    // "vTable" pointer automatically added at index 0 (size 8, offset 0).
    // m_data starts at next aligned position: offset 8 (size 8).
    // Total size = 16 bytes. Max alignment = 8. VTable slot 0 matches function offset 0.
    verifier.classSize(classId, 16)
            .classAlignment(classId, 8)
            .fieldOffset(classId, "vTable", 0)
            .fieldOffset(classId, "m_data", 8)
            .methodOffset(classId, 0, 0);
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
    size_t parentId = parentClass->getId();

    // Create the child context structures
    MirFunction *derivedBarOverride = funcBuilder.build(types->i32(), "bar");
    MirFunction *derivedNewFunc = funcBuilder.build(types->getVoidType(), "baz");

    MirClassBuilder childBuilder(ctx);
    childBuilder.appendField(types->f64(), "m_childVal");
    childBuilder.appendMethod(derivedBarOverride);
    childBuilder.appendMethod(derivedNewFunc);
    MirClass *childClass = childBuilder.build(parentClass, "Child");
    size_t childId = childClass->getId();

    // Execute the layout compiler pass
    ClassOffsetResolver *pass = runPass<ClassOffsetResolver>(ctx);
    ClassOffsetResolverVerifier verifier(pass, ctx);

    verifier.executed().succeeded().mirModified();

    // Parent Layout check:
    // vTable ptr (0, size 8) -> m_baseVal (8, size 4) -> tail-padded to multiple of 8 = 16 bytes.
    verifier.classSize(parentId, 16)
            .classAlignment(parentId, 8)
            .fieldOffset(parentId, "vTable", 0)
            .fieldOffset(parentId, "m_baseVal", 8)
            .methodOffset(parentId, 0, 0)  // foo slot
            .methodOffset(parentId, 1, 8); // bar slot

    // Child Layout check:
    // Inherits base size 16. m_childVal (f64, size 8, align 8) can fit directly at offset 16.
    // Total child footprint size = 16 + 8 = 24 bytes.
    // VTable total slots = 3. Slot 0 (foo: 0), Slot 1 (bar override: 8), Slot 2 (baz new: 16).
    verifier.classSize(childId, 24)
            .classAlignment(childId, 8)
            .fieldOffset(childId, "vTable", 0)
            .fieldOffset(childId, "m_baseVal", 8)
            .fieldOffset(childId, "m_childVal", 16)
            .methodOffset(childId, 0, 0)
            .methodOffset(childId, 1, 8)
            .methodOffset(childId, 2, 16);
}