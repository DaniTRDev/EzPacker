#include "Targets/X86_64/X86_64CoffBinaryDesc.h"
#include "Helpers.h"

namespace EzTriple
{

X86_64CoffBinaryDesc::X86_64CoffBinaryDesc(std::pmr::memory_resource *alloc) :
    m_alloc(alloc), m_sections(alloc)
{
}

void X86_64CoffBinaryDesc::initialize()
{
    m_sections.clear();
    Helpers::ObjectFormat::CreateCoffSections(m_sections, m_alloc);
}

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
