#include "CodeEmitterContext.h"
#include <memory_resource>

CodeEmitterContext::CodeEmitterContext(DiagnosticCollector *diagCollector, std::pmr::memory_resource *alloc) :
    m_diagCollector(diagCollector), m_alloc(alloc), m_currentFuncLabels(alloc), m_currentFuncRelocs(alloc),
    m_labels(alloc), m_relocations(alloc)
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
    for (auto &[addr, reloc] : m_currentFuncRelocs)
    {
        if (reloc != nullptr)
        {
            pAlloc.delete_object(reloc);
        }
    }
    m_currentFuncRelocs.clear();

    // Deallocate persistent module-wide relocations
    for (auto &[addr, reloc] : m_relocations)
    {
        if (reloc != nullptr)
        {
            pAlloc.delete_object(reloc);
        }
    }
    m_relocations.clear();
}

CodeLabel *CodeEmitterContext::getOrCreateLabel(MirId id, const std::string_view &name)
{
    auto it = m_currentFuncLabels.find(id);
    if (it != m_currentFuncLabels.end())
    {
        return it->second;
    }

    std::pmr::polymorphic_allocator<> pAlloc(m_alloc);
    CodeLabel *newLabel = pAlloc.new_object<CodeLabel>();
    newLabel->m_id = id;
    newLabel->m_name = name;

    m_currentFuncLabels.insert({ id, newLabel });
    return newLabel;
}

CodeRelocation *CodeEmitterContext::addReloc(MirReference *srcRef, uint64_t address)
{
    auto it = m_currentFuncRelocs.find(address);
    if (it != m_currentFuncRelocs.end())
    {
        it->second->m_srcRef = srcRef;
        it->second->m_address = address;
        return it->second;
    }

    std::pmr::polymorphic_allocator<> pAlloc(m_alloc);
    CodeRelocation *newReloc = pAlloc.new_object<CodeRelocation>();
    newReloc->m_srcRef = srcRef;
    newReloc->m_address = address;

    m_currentFuncRelocs.insert({ address, newReloc });
    return newReloc;
}

CodeRelocation *CodeEmitterContext::getReloc(uint64_t address)
{
    // Search current function local relocations first
    auto it = m_currentFuncRelocs.find(address);
    if (it != m_currentFuncRelocs.end())
    {
        return it->second;
    }

    // Search module-wide aggregated table
    auto globalIt = m_relocations.find(address);
    if (globalIt != m_relocations.end())
    {
        return globalIt->second;
    }

    return nullptr;
}

void CodeEmitterContext::resetFuncState(MirFunction *currentFunc)
{
    if (currentFunc != nullptr)
    {
        // Flush function labels to persistent storage
        m_labels.insert({ currentFunc, std::move(m_currentFuncLabels) });
    }

    // Merge function relocations into module-wide table
    for (auto &[addr, reloc] : m_currentFuncRelocs)
    {
        m_relocations.insert_or_assign(addr, reloc);
    }

    // Reset local lookup maps for the next function emission
    m_currentFuncLabels = std::pmr::unordered_map<MirId, CodeLabel *>(m_alloc);
    m_currentFuncRelocs = std::pmr::unordered_map<uint64_t, CodeRelocation *>(m_alloc);
}