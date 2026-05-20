#include "CodeBuffer.h"

CodeSection *
CodeBuffer::createSection(CodeSectionFlags flags, ConstantArray<uint8_t> *data, const std::string_view &name)
{
    auto it = m_sectionMap.find(name);
    if (it != m_sectionMap.end())
        return it->second;

    ConstantArray<char> copiedName = m_sectionNamePool.createConstantArray(name.length() + 1);
    std::copy_n(name.data(), name.length(), copiedName.m_elems);
    copiedName.m_elems[name.length()] = '\0';

    CodeSection *section = m_sections->m_owner->createAndAppendToListBack<CodeSection>(
            m_sections,
            copiedName.m_elems,
            flags,
            data,
            m_relocationPool.createLinkedList<CodeRelocation>());
    m_sectionMap[copiedName.m_elems] = section;

    return section;
}

CodeSection *CodeBuffer::getSection(const std::string_view &name)
{
    auto it = m_sectionMap.find(name);
    if (it == m_sectionMap.end())
    {
        return nullptr;
    }

    return it->second;
}

const TypedPool &CodeBuffer::getRelocationPool() const { return m_relocationPool; }

const TypedPool &CodeBuffer::getSectionNamePool() const { return m_sectionNamePool; }

const TypedPool &CodeBuffer::getSectionsPool() const { return m_sectionsPool; }

TypedPoolLinkedList<CodeSection> *CodeBuffer::getSectionList() const { return m_sections; }

const std::map<std::string_view, CodeSection *> &CodeBuffer::getSectionMap() const { return m_sectionMap; }
