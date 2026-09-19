#ifndef EZMIR_MIR_TARGET_INSTRUCTION_DESC_H
#define EZMIR_MIR_TARGET_INSTRUCTION_DESC_H

#include "EzMirCommon.h"
#include "MirInstructionMetadata.h"
#include "Operand/MirRegisterReference.h"

class MirRegisterClass;

/**
 * Target machine instruction descriptor storing backend metadata (assembly name, target opcode ID,
 * explicit operand dataflow flags, operand register class constraints, implicit hardware register defs/uses,
 * and target flags).
 */
class MirTargetInstructionDesc
{
  public:
    /**
     * Constructs a target instruction descriptor with name, target opcode ID, operand flags, operand classes,
     * implicit defs/uses, and target instruction flags.
     */
    MirTargetInstructionDesc(const char *name,
                             size_t id,
                             std::initializer_list<MirOperandFlag> operandFlags = {},
                             std::initializer_list<MirRegisterClass *> operandClasses = {},
                             std::initializer_list<MirRegisterRef> implicitDefs = {},
                             std::initializer_list<MirRegisterRef> implicitUses = {},
                             MirInstructionFlags targetFlags = MirInstructionFlags::None);

    /**
     * Backward-compatible constructor without operand classes.
     */
    MirTargetInstructionDesc(const char *name,
                             size_t id,
                             std::initializer_list<MirOperandFlag> operandFlags,
                             std::initializer_list<MirRegisterRef> implicitDefs,
                             std::initializer_list<MirRegisterRef> implicitUses,
                             MirInstructionFlags targetFlags = MirInstructionFlags::None);

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
     * Returns the list of register class constraints for explicit instruction arguments.
     */
    const std::vector<MirRegisterClass *> &getOperandClasses() const;

    /**
     * Returns the register class constraint for the operand at index, or nullptr.
     */
    MirRegisterClass *getOperandClass(size_t index) const;

    /**
     * Sets the register class constraint for the operand at index.
     */
    void setOperandClass(size_t index, MirRegisterClass *regClass);

    /**
     * Sets the full vector of register class constraints.
     */
    void setOperandClasses(std::vector<MirRegisterClass *> classes);

    /**
     * Returns the list of implicit hardware register definitions (DEF) modified by this instruction.
     */
    const std::vector<MirRegisterRef> &getImplicitDefs() const;

    /**
     * Returns the list of implicit hardware register uses (USE) consumed by this instruction.
     */
    const std::vector<MirRegisterRef> &getImplicitUses() const;

    /**
     * Returns instruction semantic and behavioral flags.
     */
    MirInstructionFlags getTargetFlags() const;

    /**
     * Sentinel indicating that no encoding table entry is associated with this descriptor.
     */
    static constexpr size_t INVALID_ENCODING_ID = 0;

    /**
     * Returns the index into the target's generated encoding table, or INVALID_ENCODING_ID.
     */
    size_t getEncodingId() const;

    /**
     * Associates this descriptor with a generated encoding table entry.
     */
    void setEncodingId(size_t id);

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
     * Explicit operand register class constraints.
     */
    std::vector<MirRegisterClass *> m_operandClasses;

    /**
     * Hardware registers implicitly written/clobbered.
     */
    std::vector<MirRegisterRef> m_implicitDefs;

    /**
     * Hardware registers implicitly read.
     */
    std::vector<MirRegisterRef> m_implicitUses;

    /**
     * Target instruction flags.
     */
    MirInstructionFlags m_targetFlags{ MirInstructionFlags::None };

    /**
     * Index into the target encoding table (0 = unassigned).
     */
    size_t m_encodingId{ INVALID_ENCODING_ID };
};

#endif // EZMIR_MIR_TARGET_INSTRUCTION_DESC_H