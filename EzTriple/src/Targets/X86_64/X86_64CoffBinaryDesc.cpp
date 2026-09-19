#include "Targets/X86_64/X86_64CoffBinaryDesc.h"
#include "Helpers.h"

namespace EzTriple
{

/**
 * Creates the descriptor and its PMR-backed section table.
 */
X86_64CoffBinaryDesc::X86_64CoffBinaryDesc(std::pmr::memory_resource *alloc) : m_alloc(alloc), m_sections(alloc) {}

/**
 * Rebuilds the section table with the standard COFF sections (.text, .data, ...).
 */
void X86_64CoffBinaryDesc::initialize()
{
    m_sections.clear();
    Helpers::ObjectFormat::CreateCoffSections(m_sections, m_alloc);
}

/**
 * Looks up a section by type, returning nullptr when the descriptor does not define it.
 */
CodeSection *X86_64CoffBinaryDesc::getSection(SectionType type)
{
    auto it = m_sections.find(type);
    if (it != m_sections.end())
    {
        return it->second;
    }
    return nullptr;
}

} // namespace EzTriple
