#include "Function/MirFunction.h"

MirFunction::MirFunction(MirBlock *entryPoint,
                         MirType *returnType,
                         MirFunctionStackFrame *stackFrame,
                         size_t id,
                         TypedPoolLinkedList<MirBlock> *blocks,
                         TypedPoolLinkedList<MirOperand *> *parameters,
                         const char *name) :
    m_entryPoint(entryPoint), m_stackFrame(stackFrame), m_returnType(returnType), m_id(id), m_blocks(blocks),
    m_parameters(parameters), m_name(name)
{
}

const char *MirFunction::getName() { return m_name; }

MirBlock *MirFunction::getEntryPoint() { return m_entryPoint; }

MirFunctionStackFrame *MirFunction::getStackFrame() { return m_stackFrame; }

size_t MirFunction::getId() { return m_id; }

MirType *MirFunction::getReturnType() { return m_returnType; }

TypedPoolLinkedList<MirBlock> *MirFunction::getBlocks() { return m_blocks; }

TypedPoolLinkedList<MirOperand *> *MirFunction::getParameters() { return m_parameters; }

void MirFunction::appendParameter(MirRegister *param, const char *name)
{
    m_parameters->createAndAppendBack(param);
}
