#ifndef EZPACKER_EXPANDSCALARACTION_H
#define EZPACKER_EXPANDSCALARACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/Expand/MirExpansionRuleRegistry.h"
#include "Legalizer/MirLegalizer.h"

namespace LegalizeActions
{
/**
 * This action will expand an unsupported bigger type into smaller supported types. This expansion fully depends on the
 * architecture and the operations. Linear operations (addition, substraction) can be expanded into a low+high set of
 * operations with smaller types and only the carry bit needs to be taken care of.
 *
 * For non-linear operations like multiplication or division, this is a much more complex process that needs to perform
 * the operation straight without recurring to HW's instructions at all.
 */
extern LegalizationResult ExpandScalar(LegalizeCtx &ctx);

}; // namespace LegalizeActions

#endif // EZPACKER_EXPANDSCALARACTION_H
