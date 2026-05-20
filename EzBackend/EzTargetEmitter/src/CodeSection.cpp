#include "CodeSection.h"

CodeSection::CodeSection(const char *name,
                         CodeSectionFlags flags,
                         ConstantArray<uint8_t> *data,
                         TypedPoolLinkedList<CodeRelocation> *relocations) :
    m_name(name), m_flags(flags), m_relocations(relocations), m_data(data)
{
}

bool CodeSection::addReloc(const CodeRelocation &reloc)
{
    if (getRelocationAtAddr(reloc.m_offset))
    {
        throw std::runtime_error("Internal Compiler Error: Relocation already exists");
    }

    m_relocations->m_owner->createAndAppendToListBack<CodeRelocation>(m_relocations, reloc);
    return true;
}

const char *CodeSection::getName() const { return m_name; }

CodeRelocation *CodeSection::getRelocationAtAddr(uint64_t address) const
{
    for (CodeRelocation *reloc : *m_relocations)
    {
        if (reloc->m_offset == address)
            return reloc;
    }
    return nullptr;
}

ConstantArray<uint8_t> *CodeSection::getData() const { return m_data; }

CodeSectionFlags CodeSection::getFlags() const { return m_flags; }

TypedPoolLinkedList<CodeRelocation> *CodeSection::getRelocations() const { return m_relocations; }
