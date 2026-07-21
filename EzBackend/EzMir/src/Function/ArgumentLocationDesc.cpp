#include "Function/ArgumentLocationDesc.h"

ArgumentLocationDesc::ArgumentLocationDesc(ArgLocationType type, StorageT storage) : m_type(type), m_storage(storage) {}

ArgumentLocationDesc ArgumentLocationDesc::Reg(PhysicalRegId regId, size_t sizeInBytes)
{
    return ArgumentLocationDesc(ArgLocationType::Register, RegLoc{ .m_regId = regId, .m_sizeBytes = sizeInBytes });
}
ArgumentLocationDesc ArgumentLocationDesc::Indirect(bool byVal, bool copyOnReg, size_t size, PhysicalRegId ptrStorage)
{
    return ArgumentLocationDesc(ArgLocationType::Indirect,
                                IndirectLoc{ .m_isByVal = byVal,
                                             .m_copyOnReg = copyOnReg,
                                             .m_size = size,
                                             .m_pointerStorage = ptrStorage });
}
ArgumentLocationDesc ArgumentLocationDesc::Split(const std::vector<SplitPiece> &pieces)
{
    return ArgumentLocationDesc(ArgLocationType::Split, SplitLoc{ .m_parts = std::move(pieces) });
}

ArgumentLocationDesc ArgumentLocationDesc::Stack(int64_t offset, size_t sizeInBytes)
{
    return ArgumentLocationDesc(ArgLocationType::Stack,
                                StackLoc{ .m_frameOffset = offset, .m_sizeBytes = sizeInBytes });
}

ArgLocationType ArgumentLocationDesc::getType() const { return m_type; }

const IndirectLoc &ArgumentLocationDesc::getIndirect() const
{
    if (m_type != ArgLocationType::Indirect)
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Indirect from invalid variant state.");

    return std::get<IndirectLoc>(m_storage);
}

const RegLoc &ArgumentLocationDesc::getReg() const
{
    if (m_type != ArgLocationType::Register)
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Reg from invalid variant state.");

    return std::get<RegLoc>(m_storage);
}

const SplitLoc &ArgumentLocationDesc::getSplit() const
{
    if (m_type != ArgLocationType::Split)
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Split from invalid variant state.");

    return std::get<SplitLoc>(m_storage);
}

const StackLoc &ArgumentLocationDesc::getStack() const
{
    if (m_type != ArgLocationType::Stack)
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Stack from invalid variant state.");

    return std::get<StackLoc>(m_storage);
}
