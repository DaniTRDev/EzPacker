#ifndef EZPACKER_SELECTREGISTERCLASSACTION_H
#define EZPACKER_SELECTREGISTERCLASSACTION_H

#include "EzTripleCommon.h"
#include "InstructionSelector/MirInstructionSelector.h"

namespace SelectorActions
{
/**
 * Action dedicated solely to assigning MirRegisterClass constraints to operands
 * of the current instruction (including base/index registers in memory operands). If the operand is a memory operand,
 * the assigned class will be the one of the base, if any.
 *
 * It does NOT mutate the instruction opcode or delete instructions.
 *
 * If an element is NULLPTR, the target operand will be skipped (used for immediates or other non-register operands).
 *
 * If an operand has already been selected, a WARNING will be thrown and the action will override it.
 */
extern InstructionSelAction SelectRegisterClass(std::vector<MirRegisterClass *> operandClasses);
}; // namespace SelectorActions

#endif // EZPACKER_SELECTREGISTERCLASSACTION_H