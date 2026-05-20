#include "TargetDesc.h"

TargetDesc::TargetDesc(ABIDesc *abi,
                       TargetEndianness endianness,
                       const std::string &targetName) :
    m_abi(abi), m_endianness(endianness), m_targetName(targetName)
{
}

ABIDesc *TargetDesc::getABI() const { return m_abi; }

TargetEndianness TargetDesc::getEndianness() const { return m_endianness; }

size_t TargetDesc::getAbiAlignment(MirType *type) const
{
    if (m_abi)
    {
        return m_abi->getAbiAlignment(type);
    }
    return 1; // Fallback fail-safe
}

const std::string &TargetDesc::getTargetName() const { return m_targetName; }
