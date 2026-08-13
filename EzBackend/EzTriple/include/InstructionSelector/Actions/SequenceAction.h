#ifndef EZPACKER_SEQUENCEACTION_H
#define EZPACKER_SEQUENCEACTION_H

#include "EzTriple.h"
#include "InstructionSelector/MirInstructionSelector.h"

namespace SelectorActions
{

/**
 * Creates an action that will create a sequence of instructions. The resulting instructions WILL SHARE the same
 * operands as the original instruction and the original instruction will be deleted.
 */
extern InstructionSelAction Sequence(std::vector<MirTargetInstructionId> targetOpcodes);
} // namespace SelectorActions

#endif // EZPACKER_SEQUENCEACTION_H