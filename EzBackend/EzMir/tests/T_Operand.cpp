#include <gtest/gtest.h> // Ensure the IDE recognises this file as a gtest source.
#include "EzMirTestSuite/EzMirTestSuite.h"

class OperandTest : public MirTestSuiteAsGtest
{
};

TEST_F(OperandTest, Integer)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.buildInt(getTypeTable()->i8(), FlexInt(uint32_t(0xDE), 8)))
            .verifyInteger(getTypeTable()->i8(), FlexInt(uint32_t(0xDE), 8));

    MirOperandVerifier(builder.buildInt(getTypeTable()->i16(), FlexInt(uint32_t(0xDEAD), 16)))
            .verifyInteger(getTypeTable()->i16(), uint32_t(0xDEAD));

    MirOperandVerifier(builder.buildInt(getTypeTable()->i32(), FlexInt(uint32_t(0xDEADC0DE))))
            .verifyInteger(getTypeTable()->i32(), uint32_t(0xDEADC0DE));
}

TEST_F(OperandTest, Double)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.buildFloat(getTypeTable()->f64(), FlexFloat(3.141516), nullptr)).verifyDouble(3.141516);
    MirOperandVerifier(builder.buildFloat(getTypeTable()->f64(), FlexFloat(1.14151617), nullptr))
            .verifyDouble(1.14151617);
}

TEST_F(OperandTest, Float)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.buildFloat(getTypeTable()->f32(), FlexFloat(3.141516f))).verifyFloat(3.141516f);
    MirOperandVerifier(builder.buildFloat(getTypeTable()->f32(), FlexFloat(2.141516f))).verifyFloat(2.141516f);
}

TEST_F(OperandTest, FloatAnySize)
{
    MirType *bigFloat = getTypeTable()->create(MirTypeKind::FloatingPoint, 128, {}, "f128");
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.buildFloat(bigFloat, FlexFloat("2.7182818284590452353602874713526625", 128, 10)))
            .verifyFloatAnySize(bigFloat, "2.7182818284590452353602874713526625");

    MirOperandVerifier(builder.buildFloat(bigFloat, FlexFloat("2.7182818284590452353602874713526625", 128)))
            .verifyFloatAnySize(bigFloat, "2.7182818284590452353602874713526625");
}

TEST_F(OperandTest, Reference)
{
    // We need to make builders be saved in variables, otherways the debugger doesn't know what's going on and can't see
    // the value of each variable properly. Seems to be a bug that only affects VSCode...
    MirOperandBuilder builder(getBuilderCtx());
    MirType *funcType = getTestFunc()->getType();

    MirOperandVerifier op1(builder.buildRef(getTestFunc()->getEntryPoint()));
    op1.verifyReference(MIRID_INVALID, MirReferenceType::Block);
    op1.mirTypeVerifier().id(getTypeTable()->getPtr(getTypeTable()->getVoidType())->getId());

    MirOperandVerifier op2(builder.buildRef(getTestFunc()));
    op2.verifyReference(MIRID_INVALID, MirReferenceType::Function);
    op2.mirTypeVerifier().id(getTypeTable()->getPtr(funcType)->getId());
}

TEST_F(OperandTest, Register)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.buildPhysReg(getTypeTable()->f32()))
            .verifyRegister(getTypeTable()->f32(), false, MIRID_INVALID);

    MirOperandVerifier(builder.buildVReg(getTypeTable()->f32()))
            .verifyRegister(getTypeTable()->f32(), true, MIRID_INVALID);

    MirOperandVerifier(builder.buildPhysReg(getTypeTable()->f64()))
            .verifyRegister(getTypeTable()->f64(), false, MIRID_INVALID);

    MirOperandVerifier(builder.buildPhysReg(getTypeTable()->f64()))
            .verifyRegister(getTypeTable()->f64(), false, MIRID_INVALID);
}

TEST_F(OperandTest, RuntimeSymbol)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.buildRtSymbol("mySymbol")).verifyRuntimeSymbol("mySymbol");
}

TEST_F(OperandTest, FrameIndex)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirOperandVerifier(builder.build<MirFrameIndex>(getTypeTable()->f32(), 1, nullptr))
            .verifyFrameIndex(getTypeTable()->f32(), 1);

    MirOperandVerifier(builder.build<MirFrameIndex>(getTypeTable()->f64(), 1, nullptr))
            .verifyFrameIndex(getTypeTable()->f64(), 1);

    MirOperandVerifier(builder.build<MirFrameIndex>(getTypeTable()->f32(), 2, nullptr))
            .verifyFrameIndex(getTypeTable()->f32(), 2);
}

TEST_F(OperandTest, Memory)
{
    MirOperandBuilder builder(getBuilderCtx());

    MirRegister *base = builder.buildVReg(getTypeTable()->i8());
    MirInteger *displ = builder.buildInt(getTypeTable()->i8(), FlexInt(0xDE));

    MirOperandVerifier(builder.build<MirMemory>(getTypeTable()->f32(), base, displ, nullptr))
            .verifyMemory(getTypeTable()->f32(), base, displ);

    MirOperandVerifier(builder.build<MirMemory>(getTypeTable()->f32(), nullptr, displ, nullptr))
            .verifyMemory(getTypeTable()->f32(), nullptr, displ);

    MirOperandVerifier(builder.build<MirMemory>(getTypeTable()->f32(), base, nullptr, nullptr))
            .verifyMemory(getTypeTable()->f32(), base, nullptr);
}
