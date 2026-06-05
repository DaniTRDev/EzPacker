#include "Function/MirFunction.h"

MirFunction::MirFunction(MirBlock *entryPoint,
                         MirFunctionStackFrame *stackFrame,
                         MirType *returnType,
                         size_t id,
                         SourceReference *sourceRef,
                         std::pmr::list<MirBlock *> blocks,
                         std::pmr::list<MirFuncParam *> parameters,
                         std::pmr::string name) :
    m_entryPoint(entryPoint), m_stackFrame(stackFrame), m_returnType(returnType), m_id(id), m_sourceRef(sourceRef),
    m_blocks(std::move(blocks)), m_parameters(std::move(parameters)),
    m_blockIdToBlock(m_blocks.get_allocator().resource()), m_name(std::move(name))
{
    for (auto &block : m_blocks)
    {
        m_blockIdToBlock.insert({ block->getId(), block });
    }
}

MirBlock *MirFunction::getBlock(size_t id)
{
    auto it = m_blockIdToBlock.find(id);
    if (it != m_blockIdToBlock.end())
        return it->second;

    return nullptr;
}

MirBlock *MirFunction::getEntryPoint() { return m_entryPoint; }

MirFunctionStackFrame *MirFunction::getStackFrame() { return m_stackFrame; }

size_t MirFunction::getId() { return m_id; }

MirType *MirFunction::getReturnType() { return m_returnType; }

SourceReference *MirFunction::getSourceRef() const { return m_sourceRef; }

std::pmr::list<MirBlock *> &MirFunction::getBlocks() { return m_blocks; }

std::pmr::list<MirBlock *> *MirFunction::getBlocksPtr() { return &m_blocks; }

std::pmr::list<MirFuncParam *> &MirFunction::getParameters() { return m_parameters; }

const std::pmr::string &MirFunction::getName() { return m_name; }
