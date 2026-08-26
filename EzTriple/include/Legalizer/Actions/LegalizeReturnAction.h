#ifndef EZTRIPLE_LEGALIZE_RETURN_ACTION_H
#define EZTRIPLE_LEGALIZE_RETURN_ACTION_H

#include "Legalizer/Actions/LegalizeActionCommon.h"

namespace LegalizeActions
{

/**
 * Legalizes RET instructions into token-bound PUSH_RET sequences.
 * Handles SRET memory store if the return value cannot be returned in hardware registers.
 */
LegalizationResult LegalizeReturn(LegalizeCtx &ctx);

} // namespace LegalizeActions

#endif // EZTRIPLE_LEGALIZE_RETURN_ACTION_H
