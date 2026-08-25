#include "Type/MirType.h"

MirType::MirType(MirTypeKind kind,
                 class MirTypeTable *owner,
                 size_t id,
                 size_t maxAlignmentInBytes,
                 size_t totalSize,
                 std::pmr::string name,
                 std::pmr::vector<MirType *> subTypes,
                 bool isTrivial) :
    m_kind(kind), m_owner(owner), m_id(id), m_maxAlignmentInBytes(maxAlignmentInBytes), m_totalSizeInBits(totalSize),
    m_subTypes(subTypes), m_name(name), m_isTrivial(isTrivial)
{
}

bool MirType::isTrivial() const { return m_isTrivial; }

bool MirType::isUnAligned() const { return m_maxAlignmentInBytes == 0; }

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

class MirTypeTable *MirType::getOwner() const { return m_owner; }

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

void MirType::setNonTrivial() { m_isTrivial = false; }

const std::pmr::string &MirType::getName() const { return m_name; }

const std::pmr::vector<MirType *> &MirType::getSubTypes() const { return m_subTypes; }
