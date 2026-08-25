#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Instruction/MirInstruction.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

MirBuilderContext::MirBuilderContext(CallingConvDesc *defaultCallingConv,
                                     DiagnosticCollector *diagCollector,
                                     MirTypeTable *typeTable,
                                     std::pmr::monotonic_buffer_resource *globalArena) :
    m_defaultCallingConv(defaultCallingConv), m_diagCollector(diagCollector), m_currentId(1), m_typeTable(typeTable),
    m_globalResource(globalArena), m_functions(m_globalResource), m_blockIdToBlock(m_globalResource),
    m_functionIdToFunc(m_globalResource), m_globalVarIdToGVar(m_globalResource)

{
}

bool MirBuilderContext::appendBlock(MirBlock *block)
{
    if (!block)
    {
        m_diagCollector->error("MirBuilderContext", "Could not append block because it is invalid");
        return false;
    }

    auto it = m_blockIdToBlock.find(block->getId());
    if (it != m_blockIdToBlock.end())
    {
        m_diagCollector->error("MirBuilderContext", "Could not append block because it was already appended");
        return false;
    }

    m_diagCollector->trace("MirBuilderContext", "Appended block with id: {}", block->getId());
    m_blockIdToBlock.insert({ block->getId(), block });

    return true;
}

bool MirBuilderContext::appendFunction(MirFunction *func)
{
    if (!func)
    {
        m_diagCollector->error("MirBuilderContext", "Could not append function because it is invalid");
        return false;
    }

    auto it = m_functionIdToFunc.find(func->getId());
    if (it != m_functionIdToFunc.end())
    {
        m_diagCollector->error("MirBuilderContext", "Could not append function because it was already appended");
        return false;
    }

    m_diagCollector->trace("MirBuilderContext", "Appended function: {} (id: {})", func->getName(), func->getId());
    m_functions.push_back(func);
    m_functionIdToFunc.insert({ func->getId(), func });

    return true;
}

bool MirBuilderContext::appendGlobalVar(MirGlobalVar *globalVar)
{
    if (!globalVar)
    {
        m_diagCollector->error("MirBuilderContext", "Could not append global variable because it is invalid");
        return false;
    }

    auto it = m_globalVarIdToGVar.find(globalVar->getId());
    if (it != m_globalVarIdToGVar.end())
    {
        m_diagCollector->error("MirBuilderContext", "Could not append global variable because it was already appended");
        return false;
    }

    m_diagCollector->trace("MirBuilderContext",
                           "Appended global var: {} (id: {})",
                           globalVar->getName(),
                           globalVar->getId());
    m_globalVarIdToGVar.insert({ globalVar->getId(), globalVar });
    m_globalVars.push_back(globalVar);

    return true;
}

bool MirBuilderContext::appendRegister(MirRegister *reg)
{
    if (!reg)
    {
        m_diagCollector->error("MirBuilderContext", "Could not append register because it is invalid");
        return false;
    }

    if (reg->isVirtual())
    {
        auto it = m_registerIdToRegister.find(reg->getRegId());
        if (it != m_registerIdToRegister.end())
        {
            m_diagCollector->error("MirBuilderContext", "Could not append register because it was already appended");
            return false;
        }

        m_diagCollector->trace("MirBuilderContext", "Appended register: {} (id: {})", reg->getName(), reg->getRegId());
        m_registerIdToRegister.insert({ reg->getRegId(), reg });
    }

    return true;
}

CallingConvDesc *MirBuilderContext::getDefaultCallingConvention() const { return m_defaultCallingConv; }

DiagnosticCollector *MirBuilderContext::getDiagCollector() { return m_diagCollector; }

MirBlock *MirBuilderContext::getBlockById(MirId id) const
{
    auto it = m_blockIdToBlock.find(id);
    if (it != m_blockIdToBlock.end())
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

MirId MirBuilderContext::createId() { return m_currentId++; }

MirRegister *MirBuilderContext::getRegisterById(MirId id) const
{
    auto it = m_registerIdToRegister.find(id);

    if (it != m_registerIdToRegister.end())
        return it->second;

    return nullptr;
}

MirTypeTable *MirBuilderContext::getTypeTable() { return m_typeTable; }

void MirBuilderContext::setDefaultCallingConvention(CallingConvDesc *defaultCallingConv)
{
    m_defaultCallingConv = defaultCallingConv;
}

std::pmr::monotonic_buffer_resource *MirBuilderContext::getGlobalAllocator() { return m_globalResource; }

std::pmr::list<MirFunction *> &MirBuilderContext::getFunctions() { return m_functions; }

std::pmr::list<MirGlobalVar *> &MirBuilderContext::getGlobalVars() { return m_globalVars; }
