#include "Type/MirType.h"

/**
 * Initializes a new MIR type record with its kind, owning table, ID, alignment, size, diagnostic name, and subtypes.
 */
MirType::MirType(MirTypeKind kind,
                 class MirTypeTable *owner,
                 size_t id,
                 size_t maxAlignmentInBytes,
                 size_t totalSize,
                 std::pmr::string name,
                 std::pmr::vector<MirType *> subTypes,
                 uint8_t compactId) :
    m_kind(kind), m_owner(owner), m_id(id), m_maxAlignmentInBits(maxAlignmentInBytes), m_totalSizeInBits(totalSize),
    m_compactId(compactId), m_name(std::move(name)), m_subTypes(std::move(subTypes))
{
}

/**
 * Checks whether this type has no alignment requirements (alignment = 0).
 */
bool MirType::isUnAligned() const { return m_maxAlignmentInBits == 0; }

/**
 * Retrieves the element type for array types stored as the first subtype entry.
 */
MirType *MirType::getArrayElementType() const
{
    if (m_kind == MirTypeKind::Array && !m_subTypes.empty())
    {
        return m_subTypes[0];
    }
    return nullptr;
}

/**
 * Retrieves the pointee type for pointer types stored as the first subtype entry.
 */
MirType *MirType::getPointedType() const
{
    if (m_kind == MirTypeKind::Pointer && !m_subTypes.empty())
    {
        return m_subTypes[0];
    }
    return nullptr;
}

/**
 * Retrieves the high-level category of this MIR type.
 */
MirTypeKind MirType::getKind() const { return m_kind; }

/**
 * Retrieves the type table instance managing this type record.
 */
class MirTypeTable *MirType::getOwner() const { return m_owner; }

/**
 * Retrieves the unique identifier of this type descriptor.
 */
size_t MirType::getId() const { return m_id; }

/**
 * Retrieves the maximum memory alignment requirement in bytes.
 */
size_t MirType::getMaxAlignmentInBits() const { return m_maxAlignmentInBits; }

/**
 * Retrieves the total bit width of this type.
 */
size_t MirType::getTotalSizeInBits() const { return m_totalSizeInBits; }

/**
 * Computes and returns the total byte width of this type.
 */
size_t MirType::getTotalSizeInBytes() const { return getTotalSizeInBits() / 8; }

/**
 * Retrieves the compact 8-bit type index used for fast lookups.
 */
uint8_t MirType::getCompactId() const { return m_compactId; }

/**
 * Retrieves the diagnostic/human-readable name string for this type.
 */
const std::pmr::string &MirType::getName() const { return m_name; }

/**
 * Retrieves the collection of subtype pointers (element types, pointee types, etc.).
 */
const std::pmr::vector<MirType *> &MirType::getSubTypes() const { return m_subTypes; }
