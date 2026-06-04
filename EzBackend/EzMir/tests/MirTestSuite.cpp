#include "MirTestSuite.h"

MirOperandVerifier::MirOperandVerifier(MirOperand *testedOperand) : MirVerifier(testedOperand) {}

MirOperandVerifier &MirOperandVerifier::mirType(MirType *expectedType)
{
    EXPECT_NE(getTestedObj()->getMirType(), expectedType);
    return *this;
}

MirOperandVerifier &MirOperandVerifier::type(MirOperandType expectedType)
{
    EXPECT_NE(getTestedObj()->getType(), expectedType);
    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyDouble(double val)
{
    type(MirOperandType::Double);
    EXPECT_EQ(m_testedObj->get<MirDouble>()->getValue(), val);

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyInteger(int64_t val)
{
    type(MirOperandType::Integer);
    EXPECT_EQ(m_testedObj->get<MirInteger>()->getValue(), val);

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyReference(size_t refId, MirReferenceType expectedRefType)
{
    type(MirOperandType::Reference);
    MirReference *ref = m_testedObj->get<MirReference>();

    if (refId != MIRID_INVALID)
    {
        EXPECT_EQ(ref->getRefId(), refId);
    }

    if (expectedRefType != MirReferenceType::Invalid)
    {
        EXPECT_EQ(ref->getRefType(), expectedRefType);
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyRegister(MirType *mirType, bool isVirtual, size_t id)
{
    type(MirOperandType::Register);
    MirRegister *reg = getTestedObj()->get<MirRegister>();

    if (mirType)
    {
        EXPECT_NE(reg->getMirType(), nullptr);
        EXPECT_EQ(mirType->getId(), reg->getMirType()->getId());
    }

    EXPECT_EQ(reg->isVirtual(), isVirtual);

    if (id != MIRID_INVALID)
    {
        EXPECT_EQ(reg->getRegId(), id);
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyFrameIndex(MirType *mirType, size_t frameId)
{
    type(MirOperandType::FrameIndex);
    MirFrameIndex *frameIdx = getTestedObj()->get<MirFrameIndex>();

    if (mirType)
    {
        EXPECT_NE(frameIdx->getMirType(), nullptr);
        EXPECT_EQ(mirType->getId(), frameIdx->getMirType()->getId());
    }

    if (frameId != MIRID_INVALID)
    {
        EXPECT_EQ(frameIdx->getFrameId(), frameId);
    }

    return *this;
}

MirOperandVerifier &MirOperandVerifier::verifyMemory(MirType *mirType, MirRegister *base, MirInteger *displ)
{
    type(MirOperandType::Memory);
    MirMemory *mem = getTestedObj()->get<MirMemory>();

    if (mirType)
    {
        EXPECT_NE(mem->getMirType(), nullptr);
        EXPECT_EQ(mirType->getId(), mem->getMirType()->getId());
    }

    if (base)
    {
        MirOperandVerifier baseVerifier(mem->getBase());
        baseVerifier.verifyRegister(base->getMirType(), base->isVirtual(), base->getRegId());
    }

    if (base)
    {
        MirOperandVerifier baseVerifier(mem->getDisplacement());
        baseVerifier.verifyInteger(displ->getValue());
    }

    return *this;
}

MirInstructionVerifier::MirInstructionVerifier(MirInstruction *instr) : MirVerifier(instr) {}

MirInstructionVerifier &MirInstructionVerifier::opcode(MirInstructionOpCode opcode)
{
    EXPECT_EQ(getTestedObj()->getOpCode(), opcode);
    return *this;
}

MirInstructionVerifier &MirInstructionVerifier::operandCount(size_t operandCount)
{
    EXPECT_EQ(getTestedObj()->getOperands().size(), operandCount);
    return *this;
}

MirInstructionVerifier &MirInstructionVerifier::targetId(MirTargetInstructionId id)
{
    EXPECT_EQ(getTestedObj()->getTargetId(), id);
    return *this;
}

MirOperandVerifier MirInstructionVerifier::operandVerifier(size_t operandIndex)
{
    MirInstruction *instr = getTestedObj();
    EXPECT_FALSE(instr->getOperands().size() <= operandIndex);
    return MirOperandVerifier(instr->getOperands()[operandIndex]);
}
