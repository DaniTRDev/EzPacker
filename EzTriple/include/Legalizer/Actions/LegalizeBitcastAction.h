#ifndef EZTRIPLE_LEGALIZE_BITCAST_ACTION_H
#define EZTRIPLE_LEGALIZE_BITCAST_ACTION_H

#include "Legalizer/Actions/LegalizeActionCommon.h"

class MirType;

namespace LegalizeActions
{

/**
 * Legalizes an instruction by reinterpreting operands to a bit-compatible legal type using BITCAST.
 */
LegalizationResult LegalizeBitcast(LegalizeCtx &ctx, size_t operandSlot = 0, MirType *targetType = nullptr);

} // namespace LegalizeActions

#endif // EZTRIPLE_LEGALIZE_BITCAST_ACTION_H
