#include "RiscV64ElfBinaryDesc.h"
#include "Helpers.h"

namespace EzTargets::RiscV64
{

/**
 * Creates the descriptor and its PMR-backed section table, recording the PIC preference.
 */
RiscV64ElfBinaryDesc::RiscV64ElfBinaryDesc(std::pmr::memory_resource *alloc, bool isPic) :
    BasicBinaryDesc(alloc, isPic)
{
}

/**
 * Rebuilds the section table with the standard ELF sections (.text, .data, ...).
 */
void RiscV64ElfBinaryDesc::initialize()
{
    m_sections.clear();
    Helpers::ObjectFormat::CreateElfSections(m_sections, m_alloc);
}

} // namespace EzTargets::RiscV64
