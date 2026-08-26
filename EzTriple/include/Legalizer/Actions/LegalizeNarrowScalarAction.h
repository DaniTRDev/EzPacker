#ifndef EZTRIPLE_LEGALIZE_NARROW_SCALAR_ACTION_H
#define EZTRIPLE_LEGALIZE_NARROW_SCALAR_ACTION_H

#include "Legalizer/Actions/LegalizeActionCommon.h"

class MirType;

namespace LegalizeActions
{

/**
 * Legalizes wide scalar operations (such as multi-word arithmetic on i128) by splitting
 * input operands with UNMERGE_VALUES, emitting narrow multi-word operations with carry/borrow,
 * and recombining results with MERGE_VALUES.
 */
LegalizationResult LegalizeNarrowScalar(LegalizeCtx &ctx, size_t operandSlot = 0, MirType *targetType = nullptr);

} // namespace LegalizeActions

#endif // EZTRIPLE_LEGALIZE_NARROW_SCALAR_ACTION_H
