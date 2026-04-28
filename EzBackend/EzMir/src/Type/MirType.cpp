#include "Type/MirType.h"

MirType::MirType(MirTypeKind kind, size_t id, TypedPoolSlice<MirType> *subTypes, const std::string_view &name) :
    m_kind(kind), m_id(id), m_subTypes(subTypes), m_name(name)
{
}

MirTypeKind MirType::getKind() const { return m_kind; }

size_t MirType::getId() const { return m_id; }

TypedPoolSlice<MirType> *MirType::getSubTypes() const { return m_subTypes; }

const std::string_view &MirType::getName() const { return m_name; }
