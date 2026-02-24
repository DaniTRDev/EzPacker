#include "HighLevelMir/HighLevelMirInstructionOperand.h"

HighLevelMirInstructionOperand HighLevelMirInstructionOperand::createBigIntegerImm(std::shared_ptr<mp_int> data,
                                                                                   size_t bitSize)
{
    HighLevelMirInstructionOperand result;
    result.m_type = HighLevelMirOperandType::Immediate;
    result.m_bitsize = bitSize;
    result.m_data =
            HighLevelMirImmediateOperand{ .m_type = HighLevelMirImmediateOperandType::BigInteger, .bigInteger = data };

    return result;
}

HighLevelMirInstructionOperand HighLevelMirInstructionOperand::createFloatImm(double data)
{
    HighLevelMirInstructionOperand result;
    result.m_type = HighLevelMirOperandType::Immediate;
    result.m_bitsize = sizeof(data) >> 3;
    result.m_data = HighLevelMirImmediateOperand{ .m_type = HighLevelMirImmediateOperandType::Float, ._double = data };

    return result;
}

HighLevelMirInstructionOperand HighLevelMirInstructionOperand::createIntegerImm(uint64_t data)
{
    HighLevelMirInstructionOperand result;
    result.m_type = HighLevelMirOperandType::Immediate;
    result.m_bitsize = sizeof(data) >> 3;
    result.m_data =
            HighLevelMirImmediateOperand{ .m_type = HighLevelMirImmediateOperandType::Integer, .m_integer = data };

    return result;
}

HighLevelMirInstructionOperand HighLevelMirInstructionOperand::createMem(
        uint16_t bitSize, size_t baseRegId, size_t indexRegId, int8_t scale, int64_t offset)
{
    HighLevelMirInstructionOperand result;
    result.m_type = HighLevelMirOperandType::Memory;
    result.m_bitsize = bitSize;
    result.m_data = HighLevelMirMemoryOperand{ .m_baseRegId = baseRegId,
                                               .m_indexRegId = indexRegId,
                                               .m_scale = scale,
                                               .m_offset = offset };

    return result;
}

HighLevelMirInstructionOperand HighLevelMirInstructionOperand::createRef(size_t referencedItemId)
{
    HighLevelMirInstructionOperand result;
    result.m_type = HighLevelMirOperandType::Reference;
    result.m_bitsize = 0;
    result.m_data = HighLevelReferenceOperand{ .m_referencedItemId = referencedItemId };

    return result;
}

HighLevelMirInstructionOperand HighLevelMirInstructionOperand::createReg(size_t registerId, uint16_t bitSize)
{
    HighLevelMirInstructionOperand result;
    result.m_type = HighLevelMirOperandType::Register;
    result.m_bitsize = bitSize;
    result.m_data = HighLevelRegisterOperand{ .m_registerId = registerId };

    return result;
}

HighLevelMirOperandType HighLevelMirInstructionOperand::getType() const { return m_type; }

size_t HighLevelMirInstructionOperand::getBitSize() const { return m_bitsize; }

void HighLevelMirInstructionOperand::addSourceRef(const std::shared_ptr<SourceReference> &ref)
{
    m_sourceReferences.push_back(ref);
}

void HighLevelMirInstructionOperand::setSourceRefs(const std::vector<std::shared_ptr<SourceReference>> &refs)
{
    m_sourceReferences = refs;
}

const std::vector<std::shared_ptr<SourceReference>> &HighLevelMirInstructionOperand::getSourceRefs() const
{
    return m_sourceReferences;
}

const HighLevelMirImmediateOperand *HighLevelMirInstructionOperand::getImm() const
{
    if (m_type != HighLevelMirOperandType::Immediate)
        return nullptr;

    return std::get_if<HighLevelMirImmediateOperand>(&m_data);
}

const HighLevelMirMemoryOperand *HighLevelMirInstructionOperand::getMem() const
{
    if (m_type != HighLevelMirOperandType::Memory)
        return nullptr;

    return std::get_if<HighLevelMirMemoryOperand>(&m_data);
}

const HighLevelReferenceOperand *HighLevelMirInstructionOperand::getRef() const
{
    if (m_type != HighLevelMirOperandType::Reference)
        return nullptr;

    return std::get_if<HighLevelReferenceOperand>(&m_data);
}

const HighLevelRegisterOperand *HighLevelMirInstructionOperand::getReg() const
{
    if (m_type != HighLevelMirOperandType::Register)
        return nullptr;

    return std::get_if<HighLevelRegisterOperand>(&m_data);
}
