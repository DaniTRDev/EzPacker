#include "Emitter/MirGlobalDataEmitter.h"

MirGlobalDataEmitter::MirGlobalDataEmitter(MirEmitterContext *ctx)
{
    if (!attachToContext(ctx))
    {
        throw std::runtime_error("Internal Compiler Error: Could not attach MirGlobalDataEmitter to context.");
    }
}

bool MirGlobalDataEmitter::attachToContext(MirEmitterContext *ctx)
{
    if (!ctx)
    {
        return false;
    }

    m_context = ctx;
    return true;
}

MirEmitterContext *MirGlobalDataEmitter::getContext() { return m_context; }

MirGlobalDataEntry *MirGlobalDataEmitter::createGlobalData(const void *data, size_t size, bool isReadOnly)
{
    MirGlobalDataEntry *entry = m_context->getDataEntryPool()->create<MirGlobalDataEntry>();
    entry->m_isReadOnly = isReadOnly;
    entry->m_uninitialized = false;
    entry->m_entryId = m_context->createId();
    entry->m_dataSize = size;
    entry->m_data = m_context->getEntryDataPool()->createConstantArray(size);

    if (!data)
    {
        memset(entry->m_data.m_elems, '\0', size);
        entry->m_uninitialized = true;
    }
    else
    {
        std::copy_n((char *)data, size, (char *)entry->m_data.m_elems);
    }

    return entry;
}

MirGlobalDataEntry *MirGlobalDataEmitter::createGlobalFloatingPoint(double val)
{
    return createGlobalData(&val, sizeof(val), true);
}

MirGlobalDataEntry *MirGlobalDataEmitter::createGlobalInteger(uint64_t val)
{
    return createGlobalData(&val, sizeof(val), true);
}

MirGlobalDataEntry *
MirGlobalDataEmitter::createGlobalString(const std::string_view &str, bool includeNullTerminator, bool isReadOnly)
{
    size_t totalSize = str.size() + (includeNullTerminator ? 1 : 0);
    MirGlobalDataEntry *entry = m_context->getDataEntryPool()->create<MirGlobalDataEntry>();

    entry->m_isReadOnly = isReadOnly;
    entry->m_uninitialized = false;
    entry->m_entryId = m_context->createId();
    entry->m_dataSize = totalSize;
    entry->m_data = m_context->getEntryDataPool()->createConstantArray(totalSize);
    std::copy_n(str.data(), str.size(), entry->m_data.m_elems);

    if (includeNullTerminator)
    {
        entry->m_data.m_elems[str.size()] = '\0';
    }

    return entry;
}
