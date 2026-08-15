#ifndef EZPACKER_EZTESTTRIPLEINSTRUCTIONSELECTOR_H
#define EZPACKER_EZTESTTRIPLEINSTRUCTIONSELECTOR_H

#include "EzTestTripleCommon.h"
#include "EzTestTripleInstructionSet.h"
#include "RegisterBanks/EzTestTripleRegisterBanks.h"

namespace EzTestTriple
{
/**
 * Creates the selection rules with the given ctx and selector.
 */
extern void CreateInstructionSelector(MirBuilderContext *ctx, MirInstructionSelector *selector);
}; // namespace EzTestTriple

#endif