#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Class/MirClass.h"
#include "Class/MirClassBuilder.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

class OperandTest : public MirTestSuiteAsGtest
{
};

namespace
{
::testing::AssertionResult IsInteger(MirOperand *op, MirType *expectedType, const FlexInt &expectedVal)
{
    if (!op)
        return ::testing::AssertionFailure() << "Operand is nullptr";

    if (op->getType() != MirOperandType::Integer)
        return ::testing::AssertionFailure()
                << "Expected Integer operand, got type " << static_cast<int>(op->getType());

    if (op->getMirType() != expectedType)
        return ::testing::AssertionFailure()
                << "MIR Type mismatch. Expected " << expectedType << ", got " << op->getMirType();

    auto intOp = static_cast<MirInteger *>(op);
    if (intOp->getValue() != expectedVal)
        return ::testing::AssertionFailure() << "Integer value mismatch.";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult IsFloat(MirOperand *op, MirType *expectedType, const FlexFloat &expectedVal)
{
    if (!op)
        return ::testing::AssertionFailure() << "Operand is nullptr";

    if (op->getType() != MirOperandType::FloatingPoint)
        return ::testing::AssertionFailure()
                << "Expected FloatingPoint operand, got type " << static_cast<int>(op->getType());

    if (op->getMirType() != expectedType)
        return ::testing::AssertionFailure()
                << "MIR Type mismatch. Expected " << expectedType << ", got " << op->getMirType();

    auto floatOp = static_cast<MirFloat *>(op);
    if (!(floatOp->getValue() == expectedVal))
        return ::testing::AssertionFailure() << "Float value mismatch.";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult IsReference(MirOperand *op, MirType *expectedType, MirReferenceType expectedRefType)
{
    if (!op)
        return ::testing::AssertionFailure() << "Operand is nullptr";

    if (op->getType() != MirOperandType::Reference)
        return ::testing::AssertionFailure()
                << "Expected Reference operand, got type " << static_cast<int>(op->getType());

    if (op->getMirType() != expectedType)
        return ::testing::AssertionFailure()
                << "MIR Type mismatch. Expected " << expectedType << ", got " << op->getMirType();

    auto refOp = static_cast<MirReference *>(op);
    if (refOp->getRefType() != expectedRefType)
        return ::testing::AssertionFailure()
                << "Reference Type mismatch. Expected " << static_cast<int>(expectedRefType) << ", got "
                << static_cast<int>(refOp->getRefType());

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
IsRegister(MirOperand *op, MirType *expectedType, bool expectedVirtual, size_t expectedId = MIRID_INVALID)
{
    if (!op)
        return ::testing::AssertionFailure() << "Operand is nullptr";

    if (op->getType() != MirOperandType::Register)
        return ::testing::AssertionFailure()
                << "Expected Register operand, got type " << static_cast<int>(op->getType());

    if (op->getMirType() != expectedType)
        return ::testing::AssertionFailure()
                << "MIR Type mismatch. Expected " << expectedType << ", got " << op->getMirType();

    auto regOp = static_cast<MirRegister *>(op);
    if (regOp->isVirtual() != expectedVirtual)
        return ::testing::AssertionFailure()
                << "Virtual flag mismatch. Expected " << expectedVirtual << ", got " << regOp->isVirtual();

    if (expectedId != MIRID_INVALID && regOp->getRegId() != expectedId)
        return ::testing::AssertionFailure()
                << "Register ID mismatch. Expected " << expectedId << ", got " << regOp->getRegId();

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult IsRuntimeSymbol(MirOperand *op, const std::string_view &expectedName)
{
    if (!op)
        return ::testing::AssertionFailure() << "Operand is nullptr";

    if (op->getType() != MirOperandType::RuntimeSymbol)
        return ::testing::AssertionFailure()
                << "Expected RuntimeSymbol operand, got type " << static_cast<int>(op->getType());

    auto symOp = static_cast<MirRuntimeSymbol *>(op);
    if (symOp->getSymbolName() != expectedName)
        return ::testing::AssertionFailure()
                << "Symbol Name mismatch. Expected '" << expectedName << "', got '" << symOp->getSymbolName() << "'";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
IsMemory(MirOperand *op, MirType *expectedType, MirRegister *expectedBase, int64_t expectedDispl)
{
    if (!op)
        return ::testing::AssertionFailure() << "Operand is nullptr";

    if (op->getType() != MirOperandType::Memory)
        return ::testing::AssertionFailure() << "Expected Memory operand, got type " << static_cast<int>(op->getType());

    if (op->getMirType() != expectedType)
        return ::testing::AssertionFailure()
                << "MIR Type mismatch. Expected " << expectedType << ", got " << op->getMirType();

    auto memOp = static_cast<MirMemory *>(op);
    if (memOp->getBase() != expectedBase)
        return ::testing::AssertionFailure() << "Memory Base mismatch.";

    MirInteger *displ = memOp->getDisplacement();
    if (!displ)
    {
        return ::testing::AssertionFailure() << "Expected a displacement, but got none.";
    }
    else
    {
        if (displ->getValue().getI64() != expectedDispl)
            return ::testing::AssertionFailure()
                    << "Displacement mismatch. Expected " << expectedDispl << ", got " << displ->getValue().getI64();
    }

    return ::testing::AssertionSuccess();
}

} // anonymous namespace

TEST_F(OperandTest, Integer)
{
    MirOperandBuilder builder(getBuilderCtx());
    MirTypeTable *types = getTypeTable();

    EXPECT_TRUE(IsInteger(builder.buildInt(types->i8(), FlexInt(uint32_t(0xDE), 8)),
                          types->i8(),
                          FlexInt(uint32_t(0xDE), 8)));

    EXPECT_TRUE(IsInteger(builder.buildInt(types->i16(), FlexInt(uint32_t(0xDEAD), 16)),
                          types->i16(),
                          FlexInt(uint32_t(0xDEAD), 16)));

    EXPECT_TRUE(IsInteger(builder.buildInt(types->i32(), FlexInt(uint32_t(0xDEADC0DE))),
                          types->i32(),
                          FlexInt(uint32_t(0xDEADC0DE))));
}

TEST_F(OperandTest, Double)
{
    MirOperandBuilder builder(getBuilderCtx());
    MirTypeTable *types = getTypeTable();

    EXPECT_TRUE(
            IsFloat(builder.buildFloat(types->f64(), FlexFloat(3.141516), nullptr), types->f64(), FlexFloat(3.141516)));

    EXPECT_TRUE(IsFloat(builder.buildFloat(types->f64(), FlexFloat(1.14151617), nullptr),
                        types->f64(),
                        FlexFloat(1.14151617)));
}

TEST_F(OperandTest, Float)
{
    MirOperandBuilder builder(getBuilderCtx());
    MirTypeTable *types = getTypeTable();

    EXPECT_TRUE(IsFloat(builder.buildFloat(types->f32(), FlexFloat(3.141516f)), types->f32(), FlexFloat(3.141516f)));

    EXPECT_TRUE(IsFloat(builder.buildFloat(types->f32(), FlexFloat(2.141516f)), types->f32(), FlexFloat(2.141516f)));
}

TEST_F(OperandTest, FloatAnySize)
{
    MirOperandBuilder builder(getBuilderCtx());
    MirTypeTable *types = getTypeTable();
    MirType *bigFloat = types->create(MirTypeKind::FloatingPoint, 128, {}, "f128");

    EXPECT_TRUE(IsFloat(builder.buildFloat(bigFloat, FlexFloat("2.7182818284590452353602874713526625", 128, 10)),
                        bigFloat,
                        FlexFloat("2.7182818284590452353602874713526625", 128, 10)));

    EXPECT_TRUE(IsFloat(builder.buildFloat(bigFloat, FlexFloat("2.7182818284590452353602874713526625", 128)),
                        bigFloat,
                        FlexFloat("2.7182818284590452353602874713526625", 128)));
}

TEST_F(OperandTest, Reference)
{
    MirOperandBuilder builder(getBuilderCtx());
    MirTypeTable *types = getTypeTable();
    MirType *funcType = getTestFunc()->getType();

    MirOperand *blockRef = builder.buildRef(getTestFunc()->getEntryPoint());
    EXPECT_TRUE(IsReference(blockRef, types->getPtr(types->getVoidType()), MirReferenceType::Block));

    MirOperand *funcRef = builder.buildRef(getTestFunc());
    EXPECT_TRUE(IsReference(funcRef, types->getPtr(funcType), MirReferenceType::Function));
}

TEST_F(OperandTest, Register)
{
    MirOperandBuilder builder(getBuilderCtx());
    MirTypeTable *types = getTypeTable();

    EXPECT_TRUE(IsRegister(builder.buildVReg(types->f32()), types->f32(), true));
    EXPECT_TRUE(IsRegister(builder.buildVReg(types->f32()), types->f32(), true));

    EXPECT_TRUE(IsRegister(builder.buildPhysReg(types->f64(), 3), types->f64(), false, 3));
    EXPECT_TRUE(IsRegister(builder.buildPhysReg(types->f64(), 4), types->f64(), false, 4));
}

TEST_F(OperandTest, RuntimeSymbol)
{
    MirOperandBuilder builder(getBuilderCtx());
    EXPECT_TRUE(IsRuntimeSymbol(builder.buildRtSymbol("mySymbol"), "mySymbol"));
}

TEST_F(OperandTest, Memory)
{
    MirOperandBuilder builder(getBuilderCtx());
    MirTypeTable *types = getTypeTable();

    // 1. Success case: Base MUST be a pointer type for buildMem
    MirType *ptrType = types->getPtr(types->i8());
    MirRegister *validBase = builder.buildVReg(ptrType);

    MirOperand *memOp = builder.buildMem(types->f32(), validBase, FlexInt(0xDE));
    EXPECT_TRUE(IsMemory(memOp, types->f32(), validBase, 0xDE));

    // 2. Failure case: Base is NOT a pointer type
    MirRegister *invalidBase = builder.buildVReg(types->i8()); // Just an i8, not a pointer
    MirOperand *badMemOp = builder.buildMem(types->f32(), invalidBase, FlexInt(0xDE));

    // buildMem should enforce the pointer rule and return nullptr
    EXPECT_EQ(badMemOp, nullptr);
}