#include "Targets/X86_64/X86_64ElfBinaryDesc.h"
#include "Helpers.h"

namespace EzTriple
{

/**
 * Creates the descriptor and its PMR-backed section table, recording the PIC preference.
 */
X86_64ElfBinaryDesc::X86_64ElfBinaryDesc(std::pmr::memory_resource *alloc, bool isPic) :
    m_alloc(alloc), m_isPic(isPic), m_sections(alloc)
{
}

/**
 * Rebuilds the section table with the standard ELF sections (.text, .data, ...).
 */
void X86_64ElfBinaryDesc::initialize()
{
    m_sections.clear();
    Helpers::ObjectFormat::CreateElfSections(m_sections, m_alloc);
}

/**
 * Looks up a section by type, returning nullptr when the descriptor does not define it.
 */
CodeSection *X86_64ElfBinaryDesc::getSection(SectionType type)
{
    auto it = m_sections.find(type);
    if (it != m_sections.end())
    {
        return it->second;
    }
    return nullptr;
}

} // namespace EzTriple
