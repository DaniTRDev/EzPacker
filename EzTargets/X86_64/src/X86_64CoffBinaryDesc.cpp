#include "X86_64CoffBinaryDesc.h"
#include "Helpers.h"

namespace EzTargets::X86_64
{

/**
 * Creates the non-PIC descriptor and its PMR-backed section table.
 */
X86_64CoffBinaryDesc::X86_64CoffBinaryDesc(std::pmr::memory_resource *alloc) : EzTriple::BasicBinaryDesc(alloc, false) {}

/**
 * Rebuilds the section table with the standard COFF sections (.text, .data, ...).
 */
void X86_64CoffBinaryDesc::initialize()
{
    m_sections.clear();
    Helpers::ObjectFormat::CreateCoffSections(m_sections, m_alloc);
}

} // namespace EzTargets::X86_64
