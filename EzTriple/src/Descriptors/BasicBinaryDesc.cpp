#include "Descriptors/BasicBinaryDesc.h"
#include "CodeSection.h"

namespace EzTriple
{

/**
 * Creates the base descriptor and its PMR-backed section table, recording the PIC preference.
 */
BasicBinaryDesc::BasicBinaryDesc(std::pmr::memory_resource *alloc, bool isPic) :
    m_alloc(alloc), m_isPic(isPic), m_sections(alloc)
{
}

/**
 * Looks up a section by type, returning nullptr when the descriptor does not define it.
 */
CodeSection *BasicBinaryDesc::getSection(SectionType type)
{
    auto it = m_sections.find(type);
    if (it != m_sections.end())
    {
        return it->second;
    }
    return nullptr;
}

} // namespace EzTriple
