#include "Block/MirBlock.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirOperands.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirType.h"

MirFunction::MirFunction(CallingConvDesc *callingConv,
                         MirFunctionStackFrame *stackFrame,
                         MirType *returnType,
                         MirType *type,
                         MirId id,
                         SourceReference *sourceRef,
                         std::pmr::string name,
                         std::pmr::memory_resource *alloc) :
    m_callingConv(callingConv), m_entryPoint(nullptr), m_stackFrame(stackFrame), m_returnType(returnType), m_type(type),
    m_id(id), m_sourceRef(sourceRef), m_parameters(alloc), m_blockIdToBlock(alloc), m_name(std::move(name)),
    m_usedCalleeSavedRegs(alloc)
{
}

CallingConvDesc *MirFunction::getCallingConv() const { return m_callingConv; }

const IntrusiveLinkedList<class MirBlock> &MirFunction::getBlocks() const { return m_blocks; }

IntrusiveLinkedList<class MirBlock>::const_iterator MirFunction::begin() const { return m_blocks.begin(); }

IntrusiveLinkedList<class MirBlock>::const_iterator MirFunction::end() const { return m_blocks.end(); }

MirBlock *MirFunction::getBlock(MirId id) const
{
    auto it = m_blockIdToBlock.find(id);
    if (it != m_blockIdToBlock.end())
        return it->second;

    return nullptr;
}

MirBlock *MirFunction::getEntryPoint() const { return m_entryPoint; }

MirFunction *MirFunction::getPrev() const { return m_prev; }

MirFunction *MirFunction::getNext() const { return m_next; }

MirFunctionAnalysisData *MirFunction::getAnalysisData() { return &m_analysisData; }

MirFunctionStackFrame *MirFunction::getStackFrame() const { return m_stackFrame; }

MirId MirFunction::getId() const { return m_id; }

MirType *MirFunction::getReturnType() const { return m_returnType; }

MirType *MirFunction::getType() const { return m_type; }

size_t MirFunction::getBlockCount() const { return m_blocks.size(); }

size_t MirFunction::getParamCount() const { return m_parameters.size(); }

SourceReference *MirFunction::getSourceRef() const { return m_sourceRef; }

const std::pmr::list<MirRegister *> &MirFunction::getParameters() const { return m_parameters; }

const std::pmr::string &MirFunction::getName() const { return m_name; }

const std::pmr::vector<MirRegisterRef> &MirFunction::getUsedCalleeSavedRegs() const { return m_usedCalleeSavedRegs; }

bool MirFunction::appendBlock(MirBlock *block)
{
    if (m_blockIdToBlock.contains(block->getId()))
        return false;

    m_blockIdToBlock.insert({ block->getId(), block });
    m_blocks.push_back(block);

    return true;
}

void MirFunction::addCalleeSavedRegUse(const MirRegisterRef &reg) { m_usedCalleeSavedRegs.push_back(reg); }

void MirFunction::setEntryPoint(MirBlock *entryPoint) { m_entryPoint = entryPoint; }

void MirFunction::setNext(MirFunction *next) { m_next = next; }

void MirFunction::setPrev(MirFunction *prev) { m_prev = prev; }
