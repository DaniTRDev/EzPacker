#ifndef EZTRIPLE_LEGALIZE_WIDEN_SCALAR_ACTION_H
#define EZTRIPLE_LEGALIZE_WIDEN_SCALAR_ACTION_H

#include "Legalizer/Actions/LegalizeActionCommon.h"

class MirType;

namespace LegalizeActions
{

/**
 * Legalizes an instruction with narrower scalar operands by promoting source operands
 * with sign/zero extensions, performing the operation at the wider legal type, and truncating
 * the destination result if needed.
 */
LegalizationResult LegalizeWidenScalar(LegalizeCtx &ctx, size_t operandSlot = 0, MirType *targetType = nullptr);

} // namespace LegalizeActions

#endif // EZTRIPLE_LEGALIZE_WIDEN_SCALAR_ACTION_H
