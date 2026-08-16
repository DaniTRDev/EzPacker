#ifndef EZMIR_MIR_TARGET_INSTRUCTION_DESC_H
#define EZMIR_MIR_TARGET_INSTRUCTION_DESC_H

#include "EzMirCommon.h"
#include "MirInstructionMetadata.h"
#include "Operand/MirRegisterReference.h"

/**
 * Class used to define the bare minimum things EzMir needs to know about target instructions to work properly.
 */
class MirTargetInstructionDesc
{
  public:
    MirTargetInstructionDesc(const char *name,
                             size_t id,
                             std::initializer_list<MirOperandFlag> operandFlags = {},
                             std::initializer_list<MirRegisterRef> implicitDefs = {},
                             std::initializer_list<MirRegisterRef> implicitUses = {});

    /**
     * Returns the name of the target instruction.
     */
    const char *getName() const;

    /**
     * Returns the ID of the target instruction.
     */
    size_t getId() const;

    /**
     * Returns the flags for the current operands.
     */
    const std::vector<MirOperandFlag> &getOperandsFlags() const;

    /**
     * Returns the implicit def list for this instruction, if any.
     */
    const std::vector<MirRegisterRef> &getImplicitDefs() const;

    /**
     * Returns the implicit use list for this instruction, if any.
     */
    const std::vector<MirRegisterRef> &getImplicitUses() const;

  private:
    const char *m_name;
    size_t m_id;
    std::vector<MirOperandFlag> m_operandsFlags;
    std::vector<MirRegisterRef> m_implicitDefs;
    std::vector<MirRegisterRef> m_implicitUses;
};

#endif // EZMIR_MIR_TARGET_INSTRUCTION_DESC_H