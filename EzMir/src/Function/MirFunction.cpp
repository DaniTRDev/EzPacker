#include "Block/MirBlock.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirOperands.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirType.h"

MirFunction::MirFunction(CallingConvDesc *callingConv,
                         MirBlock *entryPoint,
                         MirFunctionStackFrame *stackFrame,
                         MirType *returnType,
                         MirType *type,
                         MirId id,
                         SourceReference *sourceRef,
                         std::pmr::list<MirBlock *> blocks,
                         std::pmr::list<MirRegister *> parameters,
                         std::pmr::string name) :
    m_callingConv(callingConv), m_entryPoint(entryPoint), m_stackFrame(stackFrame), m_returnType(returnType),
    m_type(type), m_id(id), m_sourceRef(sourceRef), m_blocks(std::move(blocks)), m_parameters(std::move(parameters)),
    m_blockIdToBlock(m_blocks.get_allocator().resource()), m_name(std::move(name))
{
    for (auto &block : m_blocks)
    {
        m_blockIdToBlock.insert({ block->getId(), block });
    }

    m_usedCalleeSavedRegs = std::pmr::vector<MirRegisterRef>(m_blocks.get_allocator().resource());
}

CallingConvDesc *MirFunction::getCallingConv() const { return m_callingConv; }

MirBlock *MirFunction::getBlock(MirId id) const
{
    auto it = m_blockIdToBlock.find(id);
    if (it != m_blockIdToBlock.end())
        return it->second;

    return nullptr;
}

MirBlock *MirFunction::getEntryPoint() const { return m_entryPoint; }

MirFunctionAnalysisData *MirFunction::getAnalysisData() { return &m_analysisData; }

MirFunctionStackFrame *MirFunction::getStackFrame() const { return m_stackFrame; }

MirId MirFunction::getId() const { return m_id; }

MirType *MirFunction::getReturnType() const { return m_returnType; }

MirType *MirFunction::getType() const { return m_type; }

size_t MirFunction::getParamCount() const { return m_parameters.size(); }

SourceReference *MirFunction::getSourceRef() const { return m_sourceRef; }

void MirFunction::addCalleeSavedRegUse(const MirRegisterRef &reg) { m_usedCalleeSavedRegs.push_back(reg); }

void MirFunction::setEntryPoint(MirBlock *entryPoint) { m_entryPoint = entryPoint; }

std::pmr::list<MirBlock *> &MirFunction::getBlocks() { return m_blocks; }

std::pmr::list<MirBlock *> *MirFunction::getBlocksPtr() { return &m_blocks; }

std::pmr::list<MirRegister *> &MirFunction::getParameters() { return m_parameters; }

const std::pmr::string &MirFunction::getName() { return m_name; }

const std::pmr::vector<MirRegisterRef> &MirFunction::getUsedCalleeSavedRegs() const { return m_usedCalleeSavedRegs; }
