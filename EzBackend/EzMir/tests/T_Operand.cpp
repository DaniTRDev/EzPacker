#include <gtest/gtest.h> // Ensure the IDE recognises this file as a gtest source.
#include "MirTestSuite.h"

class OperandTest : public MirTestSuiteAsGtest
{
};

TEST_F(OperandTest, Integer)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.build<MirInteger>(getTypeTable()->i8(), 0xDE, nullptr))
            .verifyInteger(getTypeTable()->i8(), 0xDE);

    MirOperandVerifier(builder.build<MirInteger>(getTypeTable()->i16(), 0xDEAD, nullptr))
            .verifyInteger(getTypeTable()->i16(), 0xDEAD);

    MirOperandVerifier(builder.build<MirInteger>(getTypeTable()->i32(), 0xDEADC0DE, nullptr))
            .verifyInteger(getTypeTable()->i32(), 0xDEADC0DE);
}

TEST_F(OperandTest, Double)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.build<MirDouble>(getTypeTable()->getFloat32Type(), 3.141516f, nullptr))
            .verifyDouble(getTypeTable()->getFloat32Type(), 3.141516f);

    MirOperandVerifier(builder.build<MirDouble>(getTypeTable()->getFloat64Type(), 3.141516, nullptr))
            .verifyDouble(getTypeTable()->getFloat64Type(), 3.141516);
}

TEST_F(OperandTest, Reference)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(
            builder.build<MirReference>(getTypeTable()->getFloat32Type(), MirReferenceType::Block, 1, nullptr))
            .verifyReference(1, MirReferenceType::Block)
            .mirTypeVerifier()
            .id(getTypeTable()->getFloat32Type()->getId());

    MirOperandVerifier(
            builder.build<MirReference>(getTypeTable()->getFloat32Type(), MirReferenceType::Function, 1, nullptr))
            .verifyReference(1, MirReferenceType::Function)
            .mirTypeVerifier()
            .id(getTypeTable()->getFloat32Type()->getId());

    MirOperandVerifier(
            builder.build<MirReference>(getTypeTable()->getFloat32Type(), MirReferenceType::DataEntry, 1, nullptr))
            .verifyReference(1, MirReferenceType::DataEntry)
            .mirTypeVerifier()
            .id(getTypeTable()->getFloat32Type()->getId());
}

TEST_F(OperandTest, Register)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.build<MirRegister>(getTypeTable()->getFloat32Type(), false, 1, nullptr))
            .verifyRegister(getTypeTable()->getFloat32Type(), false, 1);

    MirOperandVerifier(builder.build<MirRegister>(getTypeTable()->getFloat32Type(), true, 1, nullptr))
            .verifyRegister(getTypeTable()->getFloat32Type(), true, 1);

    MirOperandVerifier(builder.build<MirRegister>(getTypeTable()->getFloat64Type(), false, 1, nullptr))
            .verifyRegister(getTypeTable()->getFloat64Type(), false, 1);

    MirOperandVerifier(builder.build<MirRegister>(getTypeTable()->getFloat64Type(), false, 2, nullptr))
            .verifyRegister(getTypeTable()->getFloat64Type(), false, 2);
}

TEST_F(OperandTest, FrameIndex)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.build<MirFrameIndex>(getTypeTable()->getFloat32Type(), 1, nullptr))
            .verifyFrameIndex(getTypeTable()->getFloat32Type(), 1);

    MirOperandVerifier(builder.build<MirFrameIndex>(getTypeTable()->getFloat64Type(), 1, nullptr))
            .verifyFrameIndex(getTypeTable()->getFloat64Type(), 1);

    MirOperandVerifier(builder.build<MirFrameIndex>(getTypeTable()->getFloat32Type(), 2, nullptr))
            .verifyFrameIndex(getTypeTable()->getFloat32Type(), 2);
}

TEST_F(OperandTest, Memory)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirRegister *base = builder.build<MirRegister>(getTypeTable()->i8(), false, 1, nullptr);
    MirInteger *displ = builder.build<MirInteger>(getTypeTable()->i8(), 0xDE, nullptr);

    MirOperandVerifier(builder.build<MirMemory>(getTypeTable()->getFloat32Type(), base, displ, nullptr))
            .verifyMemory(getTypeTable()->getFloat32Type(), base, displ);

    MirOperandVerifier(builder.build<MirMemory>(getTypeTable()->getFloat32Type(), nullptr, displ, nullptr))
            .verifyMemory(getTypeTable()->getFloat32Type(), nullptr, displ);

    MirOperandVerifier(builder.build<MirMemory>(getTypeTable()->getFloat32Type(), base, nullptr, nullptr))
            .verifyMemory(getTypeTable()->getFloat32Type(), base, nullptr);
}