#ifndef EZPACKER_MANUALSELECTACTION_H
#define EZPACKER_MANUALSELECTACTION_H

#include "EzTriple.h"
#include "InstructionSelector/MirInstructionSelector.h"

namespace SelectorActions
{
/**
 * This action dictates that an instruction is going to be selected with the given target instruction descriptor.
 */
extern InstructionSelAction ManualAction(MirTargetInstructionDesc *desc);
}; // namespace SelectorActions

#endif // EZPACKER_MANUALSELECTACTION_H
