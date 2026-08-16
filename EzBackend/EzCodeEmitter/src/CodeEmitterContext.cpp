#include "CodeEmitterContext.h"
#include <memory_resource>

CodeEmitterContext::CodeEmitterContext(DiagnosticCollector *diagCollector,
                                       const std::pmr::unordered_map<SectionType, CodeSection *> &sections,
                                       std::pmr::memory_resource *alloc) :
    m_currentLabel(nullptr), m_diagCollector(diagCollector), m_alloc(alloc), m_currentFuncLabels(alloc),
    m_currentFuncRelocs(alloc), m_labels(alloc), m_relocations(alloc), m_sections(sections)
{
}

CodeEmitterContext::~CodeEmitterContext()
{
    std::pmr::polymorphic_allocator<> pAlloc(m_alloc);

    // Deallocate unflushed active function labels
    for (auto &[id, label] : m_currentFuncLabels)
    {
        if (label != nullptr)
        {
            pAlloc.delete_object(label);
        }
    }
    m_currentFuncLabels.clear();

    // Deallocate flushed labels across all functions
    for (auto &[func, labelMap] : m_labels)
    {
        for (auto &[id, label] : labelMap)
        {
            if (label != nullptr)
            {
                pAlloc.delete_object(label);
            }
        }
    }
    m_labels.clear();

    // Deallocate unflushed active function relocations
    for (auto &reloc : m_currentFuncRelocs)
    {
        if (reloc != nullptr)
        {
            pAlloc.delete_object(reloc);
        }
    }
    m_currentFuncRelocs.clear();

    // Deallocate persistent module-wide relocations
    for (auto &[section, relocs] : m_relocations)
    {
        for (auto &reloc : relocs)
        {
            pAlloc.delete_object(reloc);
        }
    }
    m_relocations.clear();
}

CodeLabel *CodeEmitterContext::getOrCreateLabel(CodeSection *definingSection, MirId id, const std::string_view &name)
{
    auto it = m_currentFuncLabels.find(id);
    if (it != m_currentFuncLabels.end())
    {
        // Update defining section if it was previously undefined
        if (it->second->m_definingSection == nullptr)
        {
            it->second->m_definingSection = definingSection;
        }
        return it->second;
    }

    std::pmr::polymorphic_allocator<> pAlloc(m_alloc);
    CodeLabel *newLabel = pAlloc.new_object<CodeLabel>();
    newLabel->m_definingSection = definingSection;
    newLabel->m_node = nullptr;
    newLabel->m_id = id;
    newLabel->m_currentOffset = 0;
    newLabel->m_labelAddress = definingSection ? definingSection->getCurrentOffset() : 0;
    newLabel->m_name = name;

    m_currentFuncLabels.insert({ id, newLabel });
    return newLabel;
}

CodeLabel *CodeEmitterContext::getCurrentLabel() const { return m_currentLabel; }

CodeRelocation *CodeEmitterContext::addReloc(MirReference *srcRef, TargetCodeRelocationType relocType)
{
    CodeSection *sec = getCurrentSection();

    std::pmr::polymorphic_allocator<> pAlloc(m_alloc);
    CodeRelocation *newReloc = pAlloc.new_object<CodeRelocation>();
    newReloc->m_relocType = relocType;
    newReloc->m_definingSection = sec;
    newReloc->m_srcRef = srcRef;
    newReloc->m_address = sec ? sec->getCurrentOffset() : 0;

    m_currentFuncRelocs.push_back(newReloc);
    return newReloc;
}

CodeSection *CodeEmitterContext::getCurrentSection() const
{
    if (m_currentLabel && m_currentLabel->m_definingSection)
    {
        return m_currentLabel->m_definingSection;
    }
    // Fallback to primary Text section if no label has been bound yet
    return getSection(SectionType::Text);
}

CodeSection *CodeEmitterContext::getSection(SectionType type) const { return m_sections.at(type); }

void CodeEmitterContext::bindLabel(CodeLabel *label)
{
    if (!label)
    {
        return;
    }

    m_currentLabel = label;

    if (label->m_definingSection != nullptr)
    {
        // If node has not been physically bound into the section stream, create the node
        if (label->m_node == nullptr)
        {
            label->m_node = label->m_definingSection->bindLabel(label->m_id);
            label->m_labelAddress = label->m_definingSection->getCurrentOffset();
        }
        else
        {
            // Rewind cursor to this existing label node for mid-stream appending
            label->m_definingSection->setCursor(label->m_node);
        }
    }
}

void CodeEmitterContext::resetFuncState(MirFunction *currentFunc)
{
    if (currentFunc != nullptr)
    {
        // Flush function labels to persistent storage
        m_labels.insert({ currentFunc, std::move(m_currentFuncLabels) });
    }

    // Merge function relocations into module-wide table
    for (auto &reloc : m_currentFuncRelocs)
    {
        m_relocations[reloc->m_definingSection].push_back(reloc);
    }

    // Reset local lookup maps for the next function emission
    m_currentFuncLabels = std::pmr::unordered_map<MirId, CodeLabel *>(m_alloc);
    m_currentFuncRelocs = std::pmr::vector<CodeRelocation *>(m_alloc);
    m_currentLabel = nullptr;
}