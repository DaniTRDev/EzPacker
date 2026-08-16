#include <gtest/gtest.h>
#include "../include/EzMirTestSuite.h"

class ClassTest : public MirTestSuiteAsGtest
{
  public:
  protected:
};

// 1. Verifies that a base class with no parent builds correctly with standard fields.
TEST_F(ClassTest, TestBaseClassSimpleCreation)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirClassBuilder builder(ctx);
    builder.appendField(types->i32(), "m_id");
    builder.appendField(types->getPtr(types->i8()), "m_namePtr");

    MirClass *baseClass = builder.build(nullptr, "BaseClass");

    // Verify properties using our custom Class verifier
    MirClassVerifier(baseClass)
            .className("BaseClass")
            .parentClass(nullptr)
            .fieldCount(2) // No vTable field expected because no virtual methods were appended
            .verifyField(0, "m_id", types->i32())
            .verifyField(1, "m_namePtr", types->getPtr(types->i8()))
            .vTableSize(0);

    // Verify lookup by name API functions directly
    EXPECT_NE(baseClass->getFieldByName("m_id"), nullptr);
    EXPECT_EQ(baseClass->getFieldByName("m_id")->m_id, 0);
    EXPECT_EQ(baseClass->getFieldByName("nonExistent"), nullptr);
}

// 2. Verifies that adding a method forces the automatic injection of a "vTable" field at offset 0.
TEST_F(ClassTest, TestClassWithVTableFieldGeneration)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirFunctionBuilder funcBuilder(ctx);
    MirFunction *mockFunc = funcBuilder.build(types->getVoidType(), "virtualFunc");

    MirClassBuilder builder(ctx);
    builder.appendField(types->i64(), "m_data");
    builder.appendMethod(mockFunc);

    MirClass *vClass = builder.build(nullptr, "VirtualClass");

    // Because a method was added, the builder prepends the "vTable" field array pointer first.
    MirClassVerifier(vClass)
            .className("VirtualClass")
            .fieldCount(2)
            .verifyField(0, "vTable", nullptr) // Underlying VTable array pointer field
            .verifyField(1, "m_data", types->i64())
            .vTableSize(1)
            .vTableSlot(0, mockFunc);
}

// 3. Verifies single inheritance, field preservation, and method overriding.
TEST_F(ClassTest, TestSingleInheritanceAndMethodOverriding)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();
    MirFunctionBuilder funcBuilder(ctx);

    // Create the Base parent layout
    MirFunction *baseFoo = funcBuilder.build(types->getVoidType(), "foo");
    MirFunction *baseBar = funcBuilder.build(types->i32(), "bar");

    MirClassBuilder baseBuilder(ctx);
    baseBuilder.appendField(types->i32(), "m_baseVal");
    baseBuilder.appendMethod(baseFoo);
    baseBuilder.appendMethod(baseBar);
    MirClass *parentClass = baseBuilder.build(nullptr, "Parent");

    // Create the Derived child layout
    // We want to override 'bar' (matching return type + name + parameter bounds)
    MirFunction *derivedBarOverride = funcBuilder.build(types->i32(), "bar");
    MirFunction *derivedNewFunc = funcBuilder.build(types->getVoidType(), "baz");

    MirClassBuilder childBuilder(ctx);
    childBuilder.appendField(types->f64(), "m_childVal");
    childBuilder.appendMethod(derivedBarOverride); // This should overwrite parent's slot in-place!
    childBuilder.appendMethod(derivedNewFunc);     // This should be appended to the end.

    MirClass *childClass = childBuilder.build(parentClass, "Child");

    // Verify Child structural layout constraints
    MirClassVerifier(childClass)
            .className("Child")
            .parentClass(parentClass)
            .fieldCount(3) // 1. vTable, 2. m_baseVal (inherited), 3. m_childVal (new)
            .verifyField(0, "vTable", nullptr)
            .verifyField(1, "m_baseVal", types->i32())
            .verifyField(2, "m_childVal", types->f64())
            .vTableSize(3)                     // foo (inherited), bar (overridden), baz (new)
            .vTableSlot(0, baseFoo)            // Inherited completely unchanged from Parent
            .vTableSlot(1, derivedBarOverride) // Overwritten safely inside slot 1
            .vTableSlot(2, derivedNewFunc);    // Appended at slot 2

    // Test getMethodBySignature API lookup routine
    MirClassMethod *lookupResult = childClass->getMethodBySignature(types->i32(), {}, "bar");
    ASSERT_NE(lookupResult, nullptr);
    EXPECT_EQ(lookupResult->m_func, derivedBarOverride);
    EXPECT_EQ(lookupResult->m_id, 1); // Confirms VTable ID is locked to slot 1
}