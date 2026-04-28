#include "Function/MirFunction.h"

MirFunction::MirFunction(
        MirBlock *entryPoint, size_t id, size_t returnTypeId, TypedPoolSlice<MirBlock> *blocks, TypedPoolSlice<MirOperand> *parameters) :
    m_entryPoint(entryPoint), m_id(id), m_returnTypeId(returnTypeId), m_blocks(blocks), m_parameters(parameters)
{
}

MirBlock *MirFunction::getEntryPoint() { return m_entryPoint; }

size_t MirFunction::getId() { return m_id; }

size_t MirFunction::getReturnTypeId() { return m_returnTypeId; }

TypedPoolSlice<MirBlock> *MirFunction::getBlocks() { return m_blocks; }

TypedPoolSlice<MirOperand> *MirFunction::getParameters() { return m_parameters; }
