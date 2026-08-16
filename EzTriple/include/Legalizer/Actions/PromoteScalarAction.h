#ifndef EZPACKER_PROMOTESCALARACTION_H
#define EZPACKER_PROMOTESCALARACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/MirLegalizer.h"

namespace LegalizeActions
{
/**
 * This action will PROMOTE  a type. Promotion means that a smaller non-supported type gets promoted into a bigger type
 * that is actually supported by the target architecture.
 *
 * This is done by inserting ZEXT/SEXT/FPEXT/TRUNC instructions and modifying the operands of the affected instructions.
 */
extern LegalizationResult PromoteScalar(LegalizeCtx &ctx);
}; // namespace LegalizeActions

#endif // EZPACKER_PROMOTESCALARACTION_H
