#include "Type/MirType.h"

MirType::MirType(MirTypeKind kind,
                 size_t id,
                 size_t maxAlignmentInBytes,
                 size_t totalSize,
                 std::pmr::string name,
                 std::pmr::vector<MirType *> subTypes) :
    m_kind(kind), m_id(id), m_maxAlignmentInBytes(maxAlignmentInBytes), m_totalSizeInBits(totalSize),
    m_subTypes(subTypes), m_name(name)
{
}

MirType *MirType::getArrayElementType() const
{
    if (m_kind == MirTypeKind::Array && !m_subTypes.empty())
    {
        return m_subTypes[0];
    }
    return nullptr;
}

MirType *MirType::getPointedType() const
{
    if (m_kind == MirTypeKind::Pointer && !m_subTypes.empty())
    {
        return m_subTypes[0];
    }
    return nullptr;
}

MirTypeKind MirType::getKind() const { return m_kind; }

size_t MirType::getArrayElementCount() const
{
    MirType *elemType = getArrayElementType();
    if (elemType)
    {
        return getTotalSizeInBytes() / elemType->getTotalSizeInBytes();
    }
    return 0;
}

size_t MirType::getId() const { return m_id; }

size_t MirType::getMaxAlignmentInBytes() const { return m_maxAlignmentInBytes; }

size_t MirType::getTotalSizeInBits() const { return m_totalSizeInBits; }

size_t MirType::getTotalSizeInBytes() const { return getTotalSizeInBits() / 8; }

const std::pmr::string &MirType::getName() const { return m_name; }

const std::pmr::vector<MirType *> &MirType::getSubTypes() const { return m_subTypes; }
