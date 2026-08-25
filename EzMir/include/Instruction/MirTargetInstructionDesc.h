#ifndef EZMIR_MIR_TARGET_INSTRUCTION_DESC_H
#define EZMIR_MIR_TARGET_INSTRUCTION_DESC_H

#include "EzMirCommon.h"
#include "MirInstructionMetadata.h"
#include "Operand/MirRegisterReference.h"

/**
 * Target machine instruction descriptor storing backend metadata (assembly name, target opcode ID,
 * explicit operand dataflow flags, and implicit hardware register defs/uses).
 */
class MirTargetInstructionDesc
{
  public:
    /**
     * Constructs a target instruction descriptor with name, target opcode ID, operand flags, and implicit defs/uses.
     */
    MirTargetInstructionDesc(const char *name,
                             size_t id,
                             std::initializer_list<MirOperandFlag> operandFlags = {},
                             std::initializer_list<MirRegisterRef> implicitDefs = {},
                             std::initializer_list<MirRegisterRef> implicitUses = {});

    /**
     * Returns the target machine assembly mnemonic name.
     */
    const char *getName() const;

    /**
     * Returns the target-specific numeric opcode identifier.
     */
    size_t getId() const;

    /**
     * Returns the list of operand dataflow access flags (Read/Write) for explicit instruction arguments.
     */
    const std::vector<MirOperandFlag> &getOperandsFlags() const;

    /**
     * Returns the list of implicit hardware register definitions (DEF) modified by this instruction (e.g. RAX, RDX in IDIV).
     */
    const std::vector<MirRegisterRef> &getImplicitDefs() const;

    /**
     * Returns the list of implicit hardware register uses (USE) consumed by this instruction.
     */
    const std::vector<MirRegisterRef> &getImplicitUses() const;

  private:
    /**
     * Assembly mnemonic string.
     */
    const char *m_name;

    /**
     * Target-specific opcode identifier.
     */
    size_t m_id;

    /**
     * Explicit operand dataflow directions.
     */
    std::vector<MirOperandFlag> m_operandsFlags;

    /**
     * Hardware registers implicitly written/clobbered.
     */
    std::vector<MirRegisterRef> m_implicitDefs;

    /**
     * Hardware registers implicitly read.
     */
    std::vector<MirRegisterRef> m_implicitUses;
};

#endif // EZMIR_MIR_TARGET_INSTRUCTION_DESC_H