#include "EzMirTestSuite.h"
#include "Class/MirClass.h"
#include "Class/MirClassBuilder.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

class ClassTest : public MirTestSuiteAsGtest
{
  public:
  protected:
};

namespace
{
::testing::AssertionResult IsClass(MirClass *cls,
                                   const std::string_view &expectedName,
                                   MirClass *expectedParent,
                                   size_t expectedFieldCount,
                                   size_t expectedVTableSize)
{
    if (!cls)
        return ::testing::AssertionFailure() << "Class is nullptr";

    if (cls->getName() != expectedName)
        return ::testing::AssertionFailure()
                << "Class expected name '" << expectedName << "', got '" << cls->getName() << "'";

    if (cls->getParentClass() != expectedParent)
        return ::testing::AssertionFailure()
                << "Class expected parent " << expectedParent << ", got " << cls->getParentClass();

    if (cls->getFieldCount() != expectedFieldCount)
        return ::testing::AssertionFailure()
                << "Class expected " << expectedFieldCount << " fields, got " << cls->getFieldCount();

    if (cls->getVTableSize() != expectedVTableSize)
        return ::testing::AssertionFailure()
                << "Class expected VTable size " << expectedVTableSize << ", got " << cls->getVTableSize();

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
HasField(MirClass *cls, size_t index, const std::string_view &expectedName, MirType *expectedType = nullptr)
{
    if (!cls)
        return ::testing::AssertionFailure() << "Class is nullptr";

    auto field = cls->getFieldById(index);
    if (!field)
        return ::testing::AssertionFailure() << "No field found at index " << index;

    if (field->m_name != expectedName)
        return ::testing::AssertionFailure()
                << "Field " << index << " expected name '" << expectedName << "', got '" << field->m_name << "'";

    if (expectedType && field->m_type != expectedType)
        return ::testing::AssertionFailure()
                << "Field " << index << " expected type " << expectedType << ", got " << field->m_type;

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult HasMethod(MirClass *cls, size_t index, const std::string_view &expectedName)
{
    if (!cls)
        return ::testing::AssertionFailure() << "Class is nullptr";

    auto method = cls->getMethodById(index);
    if (!method)
        return ::testing::AssertionFailure() << "No method found at index " << index;

    if (!method->m_func)
        return ::testing::AssertionFailure() << "Method at index " << index << " has a null function pointer";

    if (method->m_func->getName() != expectedName)
        return ::testing::AssertionFailure() << "Method " << index << " expected function name '" << expectedName
                                             << "', got '" << method->m_func->getName() << "'";

    return ::testing::AssertionSuccess();
}

} // anonymous namespace

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
    EXPECT_TRUE(IsClass(baseClass, "BaseClass", nullptr, 2, 0));
    EXPECT_TRUE(HasField(baseClass, 0, "m_id", types->i32()));
    EXPECT_TRUE(HasField(baseClass, 1, "m_namePtr", types->getPtr(types->i8())));

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

    // Two fields (vTable, m_data). One method (mockFunc).
    EXPECT_TRUE(IsClass(vClass, "VirtualClass", nullptr, 2, 1));
    EXPECT_TRUE(HasField(vClass, 0, "vTable", nullptr));
    EXPECT_TRUE(HasField(vClass, 1, "m_data", types->i64()));
    EXPECT_TRUE(HasMethod(vClass, 0, "virtualFunc"));
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

    EXPECT_TRUE(IsClass(childClass, "Child", parentClass, 3, 3));

    // Field Checks (Inherited VTable and Base fields first)
    EXPECT_TRUE(HasField(childClass, 0, "vTable", nullptr));
    EXPECT_TRUE(HasField(childClass, 1, "m_baseVal", types->i32()));
    EXPECT_TRUE(HasField(childClass, 2, "m_childVal", types->f64()));

    // Method Checks (Overrides maintain slot index)
    EXPECT_TRUE(HasMethod(childClass, 0, "foo"));
    EXPECT_TRUE(HasMethod(childClass, 1, "bar"));
    EXPECT_TRUE(HasMethod(childClass, 2, "baz"));

    // Test getMethodBySignature API
    MirClassMethod *lookupResult = childClass->getMethodBySignature(types->i32(), {}, "bar");
    ASSERT_NE(lookupResult, nullptr);
    EXPECT_EQ(lookupResult->m_func, derivedBarOverride);
    EXPECT_EQ(lookupResult->m_id, 1); // Confirms VTable ID is locked to slot 1
}

// 4. Verifies multi-level (deep) inheritance chain propagation
TEST_F(ClassTest, TestMultiLevelInheritanceChain)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();
    MirFunctionBuilder funcBuilder(ctx);

    // 1. GrandParent
    MirFunction *grandFunc = funcBuilder.build(types->getVoidType(), "grandFunc");
    MirClassBuilder grandBuilder(ctx);
    grandBuilder.appendField(types->i8(), "m_grandField");
    grandBuilder.appendMethod(grandFunc);
    MirClass *grandClass = grandBuilder.build(nullptr, "GrandParent");

    // 2. Parent
    MirFunction *parentFunc = funcBuilder.build(types->getVoidType(), "parentFunc");
    MirClassBuilder parentBuilder(ctx);
    parentBuilder.appendField(types->i16(), "m_parentField");
    parentBuilder.appendMethod(parentFunc);
    MirClass *parentClass = parentBuilder.build(grandClass, "Parent");

    // 3. Child
    MirFunction *childFunc = funcBuilder.build(types->getVoidType(), "childFunc");
    MirClassBuilder childBuilder(ctx);
    childBuilder.appendField(types->i32(), "m_childField");
    childBuilder.appendMethod(childFunc);
    MirClass *childClass = childBuilder.build(parentClass, "Child");

    // Verify Child inherited EVERYTHING from the whole chain
    EXPECT_TRUE(IsClass(childClass, "Child", parentClass, 4, 3));

    // Fields should be accumulated strictly in inheritance hierarchy order
    EXPECT_TRUE(HasField(childClass, 0, "vTable", nullptr));
    EXPECT_TRUE(HasField(childClass, 1, "m_grandField", types->i8()));
    EXPECT_TRUE(HasField(childClass, 2, "m_parentField", types->i16()));
    EXPECT_TRUE(HasField(childClass, 3, "m_childField", types->i32()));

    // Methods should also accumulate in hierarchy order
    EXPECT_TRUE(HasMethod(childClass, 0, "grandFunc"));
    EXPECT_TRUE(HasMethod(childClass, 1, "parentFunc"));
    EXPECT_TRUE(HasMethod(childClass, 2, "childFunc"));
}