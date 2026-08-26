#ifndef EZTRIPLE_LEGALIZE_CALL_ACTION_H
#define EZTRIPLE_LEGALIZE_CALL_ACTION_H

#include "Legalizer/Actions/LegalizeActionCommon.h"

namespace LegalizeActions
{

/**
 * Legalizes high-level CALL instructions into token-bound PUSH_ARG, CALL, and POP_RET sequences.
 * Handles SRET memory allocation if the return type cannot be returned in hardware registers.
 */
LegalizationResult LegalizeCall(LegalizeCtx &ctx);

} // namespace LegalizeActions

#endif // EZTRIPLE_LEGALIZE_CALL_ACTION_H
