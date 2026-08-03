#ifndef EZPACKER_LEGALACTION_H
#define EZPACKER_LEGALACTION_H

#include "EzTripleCommon.h"
#include "Legalizer/MirLegalizer.h"

namespace LegalizeActions
{
/**
 * This action ALWAYS returns AlreadyLegal.
 */
extern LegalizationResult Legal(LegalizeCtx &ctx);
}; // namespace LegalizeActions

#endif // EZPACKER_LEGALACTION_H
