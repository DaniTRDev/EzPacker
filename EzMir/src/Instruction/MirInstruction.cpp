#include "Instruction/MirInstruction.h"
#include "Block/MirBlock.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionRegisterInfo.h"
#include "Instruction/MirInstructionSet.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperands.h"

/**
 * Initializes a new instruction node within the owning basic block.
 */
MirInstruction::MirInstruction(MirBlock *owner,
                               MirInstructionOpCode opcode,
                               SourceReference *ref,
                               std::pmr::vector<MirOperand *> operands) :
    m_owner(owner), m_opcode(opcode), m_targetDesc(nullptr), m_sourceRef(ref), m_operands(std::move(operands))
{
}

/**
 * Checks if the instruction opcode equals the queried opcode.
 */
bool MirInstruction::hasOpcode(MirInstructionOpCode opcode) const { return m_opcode == opcode; }

/**
 * Returns true if the instruction has one or more operands.
 */
bool MirInstruction::hasOperands() const { return !m_operands.empty(); }

/**
 * Returns true if this is a selected target machine instruction (TARGET_INST with valid target descriptor).
 */
bool MirInstruction::isSelected() const
{
    return m_opcode == MirInstructionOpCode::TARGET_INST && m_targetDesc != nullptr;
}

/**
 * Checks if the instruction treats its operands as signed quantities.
 */
bool MirInstruction::isSigned() const { return getMetadata().m_flags & MirInstructionFlags::TreatAsSigned; }
/**
 * Returns true once the instruction has been unlinked from its owning block.
 */
bool MirInstruction::isErased() const { return m_owner == nullptr; }

/**
 * Returns the opcode mnemonic string.
 */
const char *MirInstruction::getOpCodeName() const { return getMetadata().m_name.data(); }

/**
 * Returns the static metadata structure for this instruction opcode.
 */
const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

/**
 * Returns the owning basic block.
 */
MirBlock *MirInstruction::getOwner() const { return m_owner; }

/**
 * Returns the preceding instruction in the basic block list.
 */
MirInstruction *MirInstruction::getPrev() const { return m_prev; }

/**
 * Returns the succeeding instruction in the basic block list.
 */
MirInstruction *MirInstruction::getNext() const { return m_next; }

/**
 * Returns the functional category of the instruction from its opcode metadata.
 */
MirInstructionCategory MirInstruction::getCategory() const { return getMetadata().m_category; }

/**
 * Returns the opcode enumeration value.
 */
MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

/**
 * Returns the abstraction tier of the instruction.
 */
MirInstructionTier MirInstruction::getTier() const { return getMetadata().m_tier; }

/**
 * Returns the behavioral flags from the opcode metadata.
 */
MirInstructionFlags MirInstruction::getFlags() const { return getMetadata().m_flags; }

/**
 * Returns the target instruction descriptor if lowered.
 */
const MirTargetInstructionDesc *MirInstruction::getTargetDesc() const { return m_targetDesc; }

/**
 * Retrieves the operand pointer at the specified index, or nullptr if out of bounds.
 */
MirOperand *MirInstruction::getOperand(size_t index) const
{
    if (index >= m_operands.size())
    {
        return nullptr;
    }
    return m_operands[index];
}

/**
 * Retrieves the const operand pointer at the specified index, or nullptr if out of bounds.
 */
const MirOperand *MirInstruction::getConstOperand(size_t index) const
{
    if (index >= m_operands.size())
    {
        return nullptr;
    }
    return m_operands[index];
}

/**
 * Resolves the dataflow access flag (Read, Write, ReadWrite) for the operand at the given index.
 * Handles target instructions, fixed signatures, and elastic variadic argument slots.
 */
MirOperandFlag MirInstruction::getOperandFlag(size_t index) const
{
    if (isSelected())
    {
        if (m_targetDesc)
        {
            const auto &flags = m_targetDesc->getOperandsFlags();
            if (index < flags.size())
            {
                return flags[index];
            }
        }
        return MirOperandFlag::Read;
    }

    const MirOperandMetadataList &opMeta = getMetadata().m_operandMeta;
    const size_t metaCount = opMeta.m_count;
    const size_t totalOperands = m_operands.size();

    if (metaCount == 0 || index >= totalOperands)
    {
        return MirOperandFlag::None;
    }

    // 1. Locate the variadic expansion slot if one exists. The slot position is encoded directly
    //    in the generated metadata by declaring that operand with ExpectedOperandType::VariadicArgs
    //    (e.g. UNMERGE_VALUES repeats a leading OUT slot, PHI/MERGE_VALUES/CALL a trailing IN slot).
    size_t varSlot = size_t(-1);
    for (size_t i = 0; i < metaCount; ++i)
    {
        if (opMeta.m_slots[i].type & ExpectedOperandType::VariadicArgs)
        {
            varSlot = i;
            break;
        }
    }

    // 2. Elastic variadic slot resolution
    if (varSlot != size_t(-1))
    {
        size_t trailingFixedCount = metaCount - 1 - varSlot;

        // Malformed operand count safety fallback
        if (totalOperands < metaCount - 1)
        {
            return (index < metaCount) ? opMeta.m_slots[index].flags : MirOperandFlag::None;
        }

        // Leading fixed operands before the variadic slice
        if (index < varSlot)
        {
            return opMeta.m_slots[index].flags;
        }
        // Trailing fixed operands after the variadic slice
        else if (index >= totalOperands - trailingFixedCount)
        {
            size_t offsetFromEnd = totalOperands - index;
            return opMeta.m_slots[metaCount - offsetFromEnd].flags;
        }
        // In the variadic expansion range (inherits Read/Write/ReadWrite from slot descriptor)
        else
        {
            return opMeta.m_slots[varSlot].flags;
        }
    }

    // 3. Standard fixed-length metadata descriptor
    if (index < metaCount)
    {
        return opMeta.m_slots[index].flags;
    }
    else if (getFlags() & MirInstructionFlags::VariadicArgs)
    {
        // Fallback for variadic instructions without explicit slot
        return MirOperandFlag::Read;
    }

    return MirOperandFlag::None;
}

/**
 * Returns the total count of operands attached to this instruction.
 */
size_t MirInstruction::getOperandCount() const { return m_operands.size(); }

/**
 * Returns the source location reference for diagnostics.
 */
SourceReference *MirInstruction::getSourceRef() const { return m_sourceRef; }

/**
 * Returns const reference to the operand list.
 */
const std::pmr::vector<MirOperand *> &MirInstruction::getOperands() const { return m_operands; }

/**
 * Collects all register definitions (DEFs) written by this instruction, including explicit write
 * operands and target implicit defs. out is cleared first.
 */
void MirInstruction::getDefinedRegisters(std::pmr::vector<MirRegisterRef> &out) const
{
    out.clear();

    for (size_t i = 0; i < m_operands.size(); ++i)
    {
        visitOperandRegisters(m_operands[i],
                              getOperandFlag(i),
                              [&out](MirRegister *reg, MirOperandFlag flag)
                              {
                                  if (flag & MirOperandFlag::Write)
                                  {
                                      out.push_back(reg->getRef());
                                  }
                              });
    }

    if (m_targetDesc)
    {
        for (const auto &impDef : m_targetDesc->getImplicitDefs())
        {
            out.push_back(impDef);
        }
    }
}

/**
 * Collects all register uses (USEs) read by this instruction, including explicit read operands,
 * memory bases and target implicit uses. out is cleared first.
 */
void MirInstruction::getUsedRegisters(std::pmr::vector<MirRegisterRef> &out) const
{
    out.clear();

    for (size_t i = 0; i < m_operands.size(); ++i)
    {
        visitOperandRegisters(m_operands[i],
                              getOperandFlag(i),
                              [&out](MirRegister *reg, MirOperandFlag flag)
                              {
                                  if (flag & MirOperandFlag::Read)
                                  {
                                      out.push_back(reg->getRef());
                                  }
                              });
    }

    if (m_targetDesc)
    {
        for (const auto &impUse : m_targetDesc->getImplicitUses())
        {
            out.push_back(impUse);
        }
    }
}

/**
 * Unlinks and erases this instruction from its owning basic block, updating register def/use tracking.
 */
void MirInstruction::eraseFromOwner()
{
    if (!m_owner)
        return;

    MirFunction *func = m_owner->getOwner();
    MirFunctionRegisterInfo *regInfo = func ? func->getRegisterInfo() : nullptr;
    if (regInfo)
    {
        for (size_t i = 0; i < m_operands.size(); ++i)
        {
            visitOperandRegisters(m_operands[i],
                                  getOperandFlag(i),
                                  [&](MirRegister *reg, MirOperandFlag flag)
                                  {
                                      if (!reg->isVirtual())
                                      {
                                          return;
                                      }
                                      if (flag & MirOperandFlag::Write)
                                      {
                                          regInfo->clearDef(reg->getRegId());
                                      }
                                      if (flag & MirOperandFlag::Read)
                                      {
                                          regInfo->removeUse(reg->getRegId(), this);
                                      }
                                  });
        }
    }

    m_owner->m_instructions.remove(this);
    m_owner = nullptr;
}

/**
 * Formats the instruction into assembly text format ("<opcode> <op0>, <op1>, ...").
 */
std::string MirInstruction::toString() const
{
    std::string res;
    res.reserve(getMetadata().m_name.size() + m_operands.size() * 16 + 1);
    res += getMetadata().m_name;

    for (auto *operand : m_operands)
    {
        res += " " + (operand ? operand->toString() : "null") + ",";
    }
    return res;
}

/**
 * Updates the instruction's opcode.
 */
void MirInstruction::setOpcode(MirInstructionOpCode opcode) { m_opcode = opcode; }

/**
 * Attaches a target machine instruction descriptor.
 */
void MirInstruction::setTargetDesc(const MirTargetInstructionDesc *desc) { m_targetDesc = desc; }

/**
 * Sets the previous instruction in the intrusive list.
 */
void MirInstruction::setPrev(MirInstruction *prev) { m_prev = prev; }

/**
 * Sets the next instruction in the intrusive list.
 */
void MirInstruction::setNext(MirInstruction *next) { m_next = next; }
