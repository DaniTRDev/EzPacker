#ifndef EZPACKER_MIRBLOCKINSTRUCTIONQUERY_H
#define EZPACKER_MIRBLOCKINSTRUCTIONQUERY_H

#include "EzCoreCommon.h"
#include "MirBlock.h"

namespace InstructionQuery
{
/**
 * Predicate used to indicate if an instruction is valid for the query.
 */
using Predicate = std::function<bool(const MirInstruction *instr)>;

/**
 * Predicate that returns true if the given instr has at least 1 operand of the given type.
 * @param opcode
 * @return
 */
inline Predicate category(MirInstructionCategory category)
{
    return [=](const MirInstruction *inst) -> bool { return inst->getMetadata().m_category == category; };
}

/**
 * Prdicate used to indicate that we want a match if the instruction does not pass the given predicate check.
 * @param predicate
 * @return
 */
inline Predicate _not(const Predicate &predicate)
{
    return [=](const MirInstruction *instr) { return !predicate(instr); };
}

/**
 * Predicate that returns true if the given instr matches the opcode.
 * @param opcode
 * @return
 */
inline Predicate opcode(MirInstructionOpCode opcode)
{
    return [=](const MirInstruction *instr) -> bool { return instr->getOpCode() == opcode; };
}

/**
 * Predicate that returns true if the given instr has at least 1 operand with the flag given.
 * @param opcode
 * @return
 */
inline Predicate operandCount(size_t count)
{
    return [=](const MirInstruction *inst) -> bool { return inst->getOperands().size() == count; };
}

/**
 * Predicate that returns true if the given instr has at least 1 operand with the flag given.
 * @param opcode
 * @return
 */
inline Predicate operandFlag(OperandFlag flag)
{
    return [=](const MirInstruction *inst) -> bool
    {
        for (const auto &opConst : inst->getMetadata().m_operandConstraints)
        {
            if (opConst.flags & flag)
                return true;
        }
        return false;
    };
}

/**
 * Predicate that returns true if the given instr has at least 1 operand of the given mir type.
 * @param opcode
 * @return
 */
inline Predicate operandMirType(MirType *type)
{
    return [=](const MirInstruction *inst) -> bool
    {
        // Check if any operand matches the type
        for (size_t i = 0; i < inst->getOperands().size(); ++i)
        {
            if (inst->getOperands()[i]->getMirType()->getId() == type->getId())
                return true;
        }
        return false;
    };
}

/**
 * Predicate that returns true if the given instr has at least 1 operand of the given type.
 * @param opcode
 * @return
 */
inline Predicate operandType(MirOperandType type)
{
    return [=](const MirInstruction *inst) -> bool
    {
        // Check if any operand matches the type
        for (size_t i = 0; i < inst->getOperands().size(); ++i)
        {
            if (inst->getOperands()[i]->getType() == type)
                return true;
        }
        return false;
    };
}
} // namespace InstructionQuery

/**
 * Class used to get a query (filtered or not) from the instruction list of a given block.
 */
class MirBlockInstructionQuery
{
  public:
    /**
     * Creates the query attached to the given block.
     * @param block
     */
    MirBlockInstructionQuery(const class MirBlock *block);

    /**
     * Appends a predicate to the query.
     * @param pred
     * @return
     */
    MirBlockInstructionQuery &predicate(const InstructionQuery::Predicate &pred);

    /**
     * Finds the FIRST instruction that matches the criteria set with predicates.
     * @return
     */
    MirInstruction *findFirst() const;

    /**
     * Collects EVERY instruction that matches the criteria set with predicates.
     * @return
     */
    std::pmr::vector<MirInstruction *> findAll() const;

    /**
     * Executes a custom lambda callback on every matching instruction without creating a new container.
     * @param callback
     * @return
     */
    void forEach(const std::function<void(MirInstruction *)> &callback) const;

  private:
    const class MirBlock *m_block;
    std::list<InstructionQuery::Predicate> m_predicates;
};

#endif // EZPACKER_MIRBLOCKINSTRUCTIONQUERY_H
