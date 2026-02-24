#include "HighLevelMir/HighLevelMirInstruction.h"

HighLevelMirInstruction::HighLevelMirInstruction(HighLevelMirOpCode opcode,
                                                 const std::vector<std::shared_ptr<SourceReference>> &sourceRefs) :
    m_opcode(opcode), m_sourceReferences(sourceRefs)
{
}

HighLevelMirInstruction &HighLevelMirInstruction::addOperand(HighLevelMirInstructionOperand operand)
{
    m_operands.push_back(std::move(operand));
    return *this;
}

const HighLevelMirMetadata &HighLevelMirInstruction::getMeta() const { return ::getMeta(m_opcode); }

HighLevelMirOpCode HighLevelMirInstruction::getOpCode() const { return m_opcode; }

const std::vector<std::shared_ptr<SourceReference>> &HighLevelMirInstruction::getSourceRefs() const
{
    return m_sourceReferences;
}

const std::vector<HighLevelMirInstructionOperand> &HighLevelMirInstruction::getOperands() const { return m_operands; }
