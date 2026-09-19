#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Instruction/MirInstruction.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

/**
 * Initializes the context, assigning IDs starting at 1 and backing all lookup tables with the
 * supplied global arena.
 */
MirBuilderContext::MirBuilderContext(CallingConvDesc *defaultCallingConv,
                                     DiagnosticCollector *diagCollector,
                                     MirTypeTable *typeTable,
                                     std::pmr::monotonic_buffer_resource *globalArena) :
    m_defaultCallingConv(defaultCallingConv), m_diagCollector(diagCollector), m_currentId(1), m_typeTable(typeTable),
    m_globalResource(globalArena), m_blockIdToBlock(m_globalResource), m_functionIdToFunc(m_globalResource),
    m_globalVarIdToGVar(m_globalResource)

{
}

/**
 * Registers a block in the ID lookup map, rejecting null pointers and duplicate IDs.
 */
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

/**
 * Registers a function in both the intrusive function list and the ID map, rejecting null
 * pointers and duplicate IDs.
 */
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

/**
 * Registers a global variable in both the global variable list and the ID map, rejecting null
 * pointers and duplicate IDs.
 */
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

/**
 * Tracks a virtual register by its ID so it can be resolved later. Physical registers are not
 * stored (no ID uniqueness is enforced for them) and are accepted as a no-op.
 */
bool MirBuilderContext::appendRegister(MirRegister *reg)
{
    if (!reg)
    {
        m_diagCollector->error("MirBuilderContext", "Could not append register because it is invalid");
        return false;
    }

    // Only virtual registers participate in ID-based lookup; physical registers are ignored.
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

/**
 * Returns the calling convention used when an entity does not specify one explicitly.
 */
CallingConvDesc *MirBuilderContext::getDefaultCallingConvention() const { return m_defaultCallingConv; }

/**
 * Returns the collector used for error and trace diagnostics raised while building MIR.
 */
DiagnosticCollector *MirBuilderContext::getDiagCollector() { return m_diagCollector; }

/**
 * Returns the mutable list of functions registered in this context.
 */
IntrusiveLinkedList<MirFunction> &MirBuilderContext::getFunctions() { return m_functions; }

/**
 * Looks up a block by ID, returning nullptr when no such block is registered.
 */
MirBlock *MirBuilderContext::getBlockById(MirId id) const
{
    auto it = m_blockIdToBlock.find(id);
    if (it != m_blockIdToBlock.end())
    {
        return it->second;
    }

    return nullptr;
}

/**
 * Looks up a function by ID, returning nullptr when no such function is registered.
 */
MirFunction *MirBuilderContext::getFuncById(MirId id) const
{
    auto it = m_functionIdToFunc.find(id);
    if (it != m_functionIdToFunc.end())
    {
        return it->second;
    }

    return nullptr;
}

/**
 * Looks up a global variable by ID, returning nullptr when no such variable is registered.
 */
MirGlobalVar *MirBuilderContext::getGVarById(MirId id) const
{
    auto it = m_globalVarIdToGVar.find(id);
    if (it != m_globalVarIdToGVar.end())
    {
        return it->second;
    }

    return nullptr;
}

/**
 * Returns the next unused ID and advances the monotonic counter.
 */
MirId MirBuilderContext::createId() { return m_currentId++; }

/**
 * Looks up a virtual register by ID, returning nullptr when it was never registered.
 */
MirRegister *MirBuilderContext::getRegisterById(MirId id) const
{
    auto it = m_registerIdToRegister.find(id);

    if (it != m_registerIdToRegister.end())
        return it->second;

    return nullptr;
}

/**
 * Returns the type table used to build and deduplicate MIR types.
 */
MirTypeTable *MirBuilderContext::getTypeTable() { return m_typeTable; }

/**
 * Replaces the calling convention applied to entities without an explicit convention.
 */
void MirBuilderContext::setDefaultCallingConvention(CallingConvDesc *defaultCallingConv)
{
    m_defaultCallingConv = defaultCallingConv;
}

/**
 * Returns the arena that owns all context-lifetime objects and lookup tables.
 */
std::pmr::monotonic_buffer_resource *MirBuilderContext::getGlobalAllocator() { return m_globalResource; }

/**
 * Returns the mutable list of global variables registered in this context.
 */
std::pmr::list<MirGlobalVar *> &MirBuilderContext::getGlobalVars() { return m_globalVars; }
