#include "Builder/MirBuilderContext.h"
#include "Type/MirTypeTable.h"

MirBuilderContext::MirBuilderContext(CallingConvDesc *defaultCallingConv,
                                     std::pmr::monotonic_buffer_resource *globalArena,
                                     const std::shared_ptr<DiagnosticCollector> &diagCollector,
                                     const std::shared_ptr<MirTypeTable> &typeTable) :
    m_defaultCallingConv(defaultCallingConv), m_currentId(1), m_globalResource(globalArena),
    m_functionResource(m_globalResource), m_functions(m_globalResource), m_blockIdToBlock(m_globalResource),
    m_classIdToClass(m_globalResource), m_functionIdToFunc(m_globalResource), m_globalVarIdToGVar(m_globalResource),
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

bool MirBuilderContext::appendClass(MirClass *_class)
{
    if (!_class)
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append class because it is invalid";
        return false;
    }

    auto it = m_classIdToClass.find(_class->getId());
    if (it != m_classIdToClass.end())
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append class because it was already appended";
        return false;
    }

    m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirBuilderContext")
            << std::pmr::string(std::format("Appended class with id: {}", _class->getId()));

    m_typeIdToClass.insert({ _class->getType()->getId(), _class });
    m_classIdToClass.insert({ _class->getId(), _class });
    m_classes.push_back(_class);

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

bool MirBuilderContext::appendGlobalVar(MirGlobalVar *globalVar)
{
    if (!globalVar)
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append global variable because it is invalid";
        return false;
    }

    auto it = m_globalVarIdToGVar.find(globalVar->getId());
    if (it != m_globalVarIdToGVar.end())
    {
        m_diagCollector->builder(DiagnosticMessageType::Diag_Error, "MirBuilderContext")
                << "Could not append global variable because it was already appended";
        return false;
    }

    m_globalVarIdToGVar.insert({ globalVar->getId(), globalVar });
    m_diagCollector->builder(DiagnosticMessageType::Diag_Trace, "MirBuilderContext") << std::pmr::string(
            std::format("Appended global var: {} (id: {})", globalVar->getName(), globalVar->getId()));
    m_globalVars.push_back(globalVar);

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

    if (reg->isVirtual())
    {
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
    }

    return true;
}

CallingConvDesc *MirBuilderContext::getDefaultCallingConvention() const { return m_defaultCallingConv; }

MirId MirBuilderContext::createId() { return m_currentId++; }

MirBlock *MirBuilderContext::getBlockById(MirId id) const
{
    auto it = m_blockIdToBlock.find(id);
    if (it != m_blockIdToBlock.end())
    {
        return it->second;
    }

    return nullptr;
}

MirClass *MirBuilderContext::getClassById(MirId id) const
{
    auto it = m_classIdToClass.find(id);
    if (it != m_classIdToClass.end())
    {
        return it->second;
    }

    return nullptr;
}

MirClass *MirBuilderContext::getClassByTypeId(MirId id) const
{
    auto it = m_typeIdToClass.find(id);
    if (it != m_typeIdToClass.end())
    {
        return it->second;
    }

    return nullptr;
}

MirFunction *MirBuilderContext::getFuncById(MirId id) const
{
    auto it = m_functionIdToFunc.find(id);
    if (it != m_functionIdToFunc.end())
    {
        return it->second;
    }

    return nullptr;
}

MirGlobalVar *MirBuilderContext::getGVarById(MirId id) const
{
    auto it = m_globalVarIdToGVar.find(id);
    if (it != m_globalVarIdToGVar.end())
    {
        return it->second;
    }

    return nullptr;
}

MirRegister *MirBuilderContext::getRegisterById(MirId id) const
{
    auto it = m_registerIdToRegister.find(id);

    if (it != m_registerIdToRegister.end())
        return it->second;

    return nullptr;
}

std::pmr::monotonic_buffer_resource *MirBuilderContext::getGlobalAllocator() { return m_globalResource; }

std::pmr::monotonic_buffer_resource *MirBuilderContext::getFuncAllocator() { return m_functionResource; }

std::pmr::list<MirClass *> &MirBuilderContext::getClasses() { return m_classes; }

std::pmr::list<MirFunction *> &MirBuilderContext::getFunctions() { return m_functions; }

std::pmr::list<MirGlobalVar *> &MirBuilderContext::getGlobalVars() { return m_globalVars; }

const std::shared_ptr<DiagnosticCollector> &MirBuilderContext::getDiagCollector() { return m_diagCollector; }

const std::shared_ptr<MirTypeTable> &MirBuilderContext::getTypeTable() { return m_typeTable; }
