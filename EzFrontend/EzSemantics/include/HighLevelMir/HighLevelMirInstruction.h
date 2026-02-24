#ifndef EZPACKER_HIGHLEVELMIRINSTRUCTION_H
#define EZPACKER_HIGHLEVELMIRINSTRUCTION_H

#include "EzSemanticsCommon.h"
#include "HighLevelMirDefs.h"
#include "HighLevelMirInstructionOperand.h"

/**
 * Since we might have a lot of instructions (even more after obfuscation passes), this class aims to allocate
 * as less heap as possible. TODO: Create a TypedPool to allocate objects near to improve CACHE access.
 */
class HighLevelMirInstruction
{
  public:
    /**
     * Creates an instruction with the given opcode.
     * @param opcode
     * @param sourceRefs
     */
    HighLevelMirInstruction(HighLevelMirOpCode opcode, const std::vector<std::shared_ptr<SourceReference>> &sourceRefs);

    /**
     * Adds an operand to this instruction. Uses a builder pattern to improve readability.
     * @param operand
     * @return HighLevelMirInstruction &
     */
    HighLevelMirInstruction &addOperand(HighLevelMirInstructionOperand operand);

    /**
     * Returns the metadata of this instruction.
     * @return const HighLevelMirMetadata &
     */
    const HighLevelMirMetadata &getMeta() const;

    /**
     * Returns the opcode.
     * @return HighLevelMirOpCode
     */
    HighLevelMirOpCode getOpCode() const;

    /**
     * Returns the source references attached to this instruction.
     * @return const std::vector<std::shared_ptr<SourceReference>> &
     */
    const std::vector<std::shared_ptr<SourceReference>> &getSourceRefs() const;

    /**
     * Returns the list of operands.
     * @return const std::vector<HighLevelMirInstructionOperand> &
     */
    const std::vector<HighLevelMirInstructionOperand> &getOperands() const;

  private:
    HighLevelMirOpCode m_opcode;
    std::vector<std::shared_ptr<SourceReference>> m_sourceReferences;
    std::vector<HighLevelMirInstructionOperand> m_operands;
};

#endif // EZPACKER_HIGHLEVELMIRINSTRUCTION_H
