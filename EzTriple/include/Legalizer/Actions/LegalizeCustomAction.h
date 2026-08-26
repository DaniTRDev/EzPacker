#ifndef EZTRIPLE_LEGALIZE_CUSTOM_ACTION_H
#define EZTRIPLE_LEGALIZE_CUSTOM_ACTION_H

#include "Legalizer/Actions/LegalizeActionCommon.h"

namespace LegalizeActions
{

/**
 * Legalizes an instruction by invoking target-specific expansion rewrite rules
 * from the target's MirExpansionRuleRegistry.
 */
LegalizationResult LegalizeCustom(LegalizeCtx &ctx);

} // namespace LegalizeActions

#endif // EZTRIPLE_LEGALIZE_CUSTOM_ACTION_H
