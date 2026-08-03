#ifndef EZPACKER_LEGALIZERETURNACTION_H
#define EZPACKER_LEGALIZERETURNACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/MirLegalizer.h"

namespace LegalizeActions
{
/**
 * This action will split a single-operand return into a PUSH_RET and a no-operand RET.
 *
 * EXAMPLE -> RET i64 %ret1
 *
 * RESULT ->
 * PUSH_RET __bindingToken %token, i64 %ret1
 * RET __bindingToken %token
 */
extern LegalizationResult LegalizeReturn(LegalizeCtx &ctx);
}; // namespace LegalizeActions

#endif // EZPACKER_LEGALIZERETURNACTION_H
