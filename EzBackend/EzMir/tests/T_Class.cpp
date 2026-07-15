#include <gtest/gtest.h>
#include "EzMirTestSuite/EzMirTestSuite.h"
#include "Printer/MirPrinter.h" // For verifying printer integration optionally

// TODO: Finish.

class ClassTest : public MirTestSuiteAsGtest
{
  public:
  protected:
    // Helper to fetch the test target type layout
    IMirTargetTypeLayout *getLayout() { return getBuilderCtx()->getTypeTable()->getTypeLayout(); }
};

// 1. Tests a standard base class with no parent
TEST_F(ClassTest, TestBaseClassSimpleLayout)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    // Field configuration: i32 (4 bytes), Pointer (8 bytes assuming 64-bit target)
    MirType *i32Type = types->i32();
    MirType *ptrType = types->getPtr(types->i8());

    // Construct the class instance
    std::pmr::vector<MirType *> fields({ i32Type, ptrType }, getGlobalArena());
    MirType *classType = types->getClass(fields, "MyBaseClass");

    // Retrieve or instantiate your class object wrapper
    // (Assuming MirClass maps directly to/is generated from its Type Struct)
    MirClass *baseClass = getMirClassRegistry()->createClass("MyBaseClass", classType);
    baseClass->addField("m_id", i32Type, 0x8);   // 8-byte offset after vptr
    baseClass->addField("m_ptr", ptrType, 0x10); // Aligned to 8-byte boundary

    // Verify properties
    MirClassVerifier(baseClass)
            .className("MyBaseClass")
            .parentClass(nullptr)
            .classType(classType)
            .fieldCount(2)
            .verifyField(0, "m_id", i32Type, 0x8)
            .verifyField(1, "m_ptr", ptrType, 0x10)
            .vTableSize(0);
}

// 2. Tests struct/class alignment & tail-padding rules via the TypeTable directly
TEST_F(ClassTest, TestClassAlignmentAndPadding)
{
    MirTypeTable *types = getTypeTable();

    // Configuration: i8 (1 byte), i64 (8 bytes), i8 (1 byte)
    // Expected Layout:
    // [0-7]   : vptr (8 bytes)
    // [8]     : i8 Field
    // [9-15]  : Padding (to align the i64 to an 8-byte boundary)
    // [16-23] : i64 Field
    // [24]    : i8 Field
    // [25-31] : Tail padding to round up the structure size to a multiple of 8 (max alignment)
    // Expected Size: 32 bytes (256 bits)
    std::pmr::vector<MirType *> fields({ types->i8(), types->i64(), types->i8() }, getGlobalArena());
    MirType *classType = types->getClass(fields, "PaddedClass");

    EXPECT_EQ(classType->getTotalSizeInBytes(), 32);
    EXPECT_EQ(classType->getTotalSizeInBits(), 256);
}

// 3. Tests Single Inheritance with layout verification and VTable slot inheritance
TEST_F(ClassTest, TestSingleInheritanceVTableOverriding)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    // Create a dummy Base class
    std::pmr::vector<MirType *> baseFields({ types->i32() }, getGlobalArena());
    MirType *baseType = types->getClass(baseFields, "Base");
    MirClass *baseClass = getMirClassRegistry()->createClass("Base", baseType);

    // Create dummy base functions for the VTable
    MirFunctionBuilder fBuilder(ctx);
    MirFunction *baseFunc1 = fBuilder.build(types->getVoidType(), "foo");
    MirFunction *baseFunc2 = fBuilder.build(types->getVoidType(), "bar");

    baseClass->addVTableSlot(baseFunc1); // Slot 0
    baseClass->addVTableSlot(baseFunc2); // Slot 1

    // Verify Base Class
    MirClassVerifier(baseClass).className("Base").vTableSize(2).vTableSlot(0, baseFunc1).vTableSlot(1, baseFunc2);

    // Create a Derived class inheriting from Base
    std::pmr::vector<MirType *> derivedFields({ types->i32(), types->i64() }, getGlobalArena());
    MirType *derivedType = types->getClass(derivedFields, "Derived");
    MirClass *derivedClass = getMirClassRegistry()->createClass("Derived", derivedType, baseClass);

    // Overridden method replacing baseFunc2 (Slot 1), and a new custom method (Slot 2)
    MirFunction *derivedOverrideFunc = fBuilder.build(types->getVoidType(), "bar_overridden");
    MirFunction *derivedNewFunc = fBuilder.build(types->getVoidType(), "baz_new");

    derivedClass->addVTableSlot(baseFunc1);           // Slot 0 (inherited)
    derivedClass->addVTableSlot(derivedOverrideFunc); // Slot 1 (overridden!)
    derivedClass->addVTableSlot(derivedNewFunc);      // Slot 2 (appended!)

    // Verify Derived Class structural invariants
    MirClassVerifier(derivedClass)
            .className("Derived")
            .parentClass(baseClass)
            .classType(derivedType)
            .vTableSize(3)
            .vTableSlot(0, baseFunc1)           // Unchanged
            .vTableSlot(1, derivedOverrideFunc) // Overridden
            .vTableSlot(2, derivedNewFunc);     // Added
}