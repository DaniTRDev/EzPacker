#ifndef EZPACKER_MIRTARGETINSTRUCTIONDESC_H
#define EZPACKER_MIRTARGETINSTRUCTIONDESC_H

#include "EzMirCommon.h"
#include "MirInstructionDefs.h"
#include "Operand/MirRegisterReference.h"

/**
 * Class used to define the bare minimum things EzMir needs to know about target instructions to work properly.
 */
class MirTargetInstructionDesc
{
  public:
    MirTargetInstructionDesc(const char *name,
                             size_t id,
                             std::initializer_list<OperandFlag> operandFlags = {},
                             std::initializer_list<class RegisterRef> implicitDefs = {},
                             std::initializer_list<class RegisterRef> implicitUses = {});

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
    const std::vector<OperandFlag> &getOperandsFlags() const;

    /**
     * Returns the implicit def list for this instruction, if any.
     */
    const std::vector<class RegisterRef> &getImplicitDefs() const;

    /**
     * Returns the implicit use list for this instruction, if any.
     */
    const std::vector<class RegisterRef> &getImplicitUses() const;

  private:
    const char *m_name;
    size_t m_id;
    std::vector<OperandFlag> m_operandsFlags;
    std::vector<class RegisterRef> m_implicitDefs;
    std::vector<class RegisterRef> m_implicitUses;
};

#endif // EZPACKER_MIRTARGETINSTRUCTIONDESC_H