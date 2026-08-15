#include "BinaryDescriptors/EzTestTripleBinaryDesc.h"

EzTestTripleBinaryDesc::EzTestTripleBinaryDesc(const Options &options, std::pmr::memory_resource *alloc) :
    m_options(options), m_alloc(alloc), m_sections(alloc)
{
}

bool EzTestTripleBinaryDesc::isLittleEndian() const { return true; }

bool EzTestTripleBinaryDesc::isPositionIndependent() const { return m_options.m_isPIC; }

const char *EzTestTripleBinaryDesc::getName() const { return "EzTestTripleBinaryDesc"; }

CodeSection *EzTestTripleBinaryDesc::getSection(SectionType type) { return m_sections.at(type); }

TargetCodeModel EzTestTripleBinaryDesc::getCodeModel() const { return m_options.m_codeModel; }

TargetObjectFormat EzTestTripleBinaryDesc::getObjectFormat() const { return m_options.m_objectFormat; }

size_t EzTestTripleBinaryDesc::getFunctionAlignment() const
{
    // 16-byte boundary aligns with standard AMD64 ABI cache-line fetch windows
    return 16;
}

size_t EzTestTripleBinaryDesc::getLoopAlignment() const { return 16; }

void EzTestTripleBinaryDesc::initialize()
{
    switch (m_options.m_objectFormat)
    {
        case TargetObjectFormat::COFF:
        {
            Helpers::ObjectFormat::CreateCoffSections(m_sections, m_alloc);
            break;
        }
        case TargetObjectFormat::ELF:
        {
            Helpers::ObjectFormat::CreateElfSections(m_sections, m_alloc);
            break;
        }
        case TargetObjectFormat::MachO:
        {
            Helpers::ObjectFormat::CreateMachoSections(m_sections, m_alloc);
            break;
        }
    }
}