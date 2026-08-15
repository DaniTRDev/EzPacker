#ifndef EZPACKER_SEQUENCEACTION_H
#define EZPACKER_SEQUENCEACTION_H

#include "EzTripleCommon.h"
#include "InstructionSelector/MirInstructionSelector.h"

namespace SelectorActions
{
/**
 * Creates an action that lowers an instruction into a sequence of target machine instructions.
 * The resulting instructions share the original instruction's operands, and all virtual registers
 * (including base/index registers within memory operands) are selected to the specified
 * register classes.
 */
extern InstructionSelAction Sequence(std::vector<MirTargetInstructionDesc *> targetDescriptors,
                                     std::initializer_list<MirRegisterClass *> operandClasses = {});
} // namespace SelectorActions

#endif // EZPACKER_SEQUENCEACTION_H