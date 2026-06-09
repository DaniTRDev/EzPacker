#include "Builder/MirBuilderContext.h"
#include "Type/MirTypeTable.h"

MirBuilderContext::MirBuilderContext(std::pmr::monotonic_buffer_resource *globalArena,
                                     const std::shared_ptr<DiagnosticCollector> &diagCollector,
                                     const std::shared_ptr<MirTypeTable> &typeTable) :
    m_currentId(1), m_globalResource(globalArena), m_functionResource(m_globalResource), m_functions(m_globalResource),
    m_functionIdToFunc(m_globalResource), m_blockIdToBlock(m_globalResource), m_globalData(m_globalResource),
    m_diagCollector(diagCollector), m_typeTable(typeTable)
{
}

bool MirBuilderContext::appendBlock(MirBlock *block)
{
    if (!block)
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append block because it is invalid";
        return false;
    }

    auto it = m_blockIdToBlock.find(block->getId());
    if (it != m_blockIdToBlock.end())
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append block because it was already appended";
        return false;
    }

    m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirBuilderContext")
            << std::pmr::string(std::format("Appended block with id: {}", block->getId()));

    m_blockIdToBlock.insert({ block->getId(), block });
    return true;
}

bool MirBuilderContext::appendFunction(MirFunction *func)
{
    if (!func)
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append function because it is invalid";
        return false;
    }

    auto it = m_functionIdToFunc.find(func->getId());
    if (it != m_functionIdToFunc.end())
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append function because it was already appended";
        return false;
    }

    m_functions.push_back(func);
    m_functionIdToFunc.insert({ func->getId(), func });
    m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirBuilderContext")
            << std::pmr::string(std::format("Appended function: {} (id: {})", func->getName(), func->getId()));

    return true;
}

bool MirBuilderContext::appendRegister(MirRegister *reg)
{
    if (!reg)
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append register because it is invalid";
        return false;
    }

    auto it = m_registerIdToRegister.find(reg->getRegId());
    if (it != m_registerIdToRegister.end())
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append register because it was already appended";
        return false;
    }

    m_registerIdToRegister.insert({ reg->getRegId(), reg });
    m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirBuilderContext")
            << std::pmr::string(std::format("Appended register: {} (id: {})", reg->getName(), reg->getRegId()));

    return true;
}

MirId MirBuilderContext::createId() { return m_currentId++; }

MirBlock *MirBuilderContext::getBlockById(size_t id) const
{
    auto it = m_blockIdToBlock.find(id);
    if (it != m_blockIdToBlock.end())
    {
        return it->second;
    }

    return nullptr;
}

MirFunction *MirBuilderContext::getFuncById(size_t id) const
{
    auto it = m_functionIdToFunc.find(id);
    if (it != m_functionIdToFunc.end())
    {
        return it->second;
    }

    return nullptr;
}

MirRegister *MirBuilderContext::getRegisterById(size_t id) const
{
    auto it = m_registerIdToRegister.find(id);

    if (it != m_registerIdToRegister.end())
        return it->second;

    return nullptr;
}

std::pmr::list<MirFunction *> &MirBuilderContext::getFunctions() { return m_functions; }

std::pmr::monotonic_buffer_resource *MirBuilderContext::getGlobalAllocator() { return m_globalResource; }

std::pmr::monotonic_buffer_resource *MirBuilderContext::getFuncAllocator() { return m_functionResource; }

const std::shared_ptr<DiagnosticCollector> &MirBuilderContext::getDiagCollector() { return m_diagCollector; }

const std::shared_ptr<MirTypeTable> &MirBuilderContext::getTypeTable() { return m_typeTable; }
