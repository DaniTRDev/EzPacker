#ifndef EZPACKER_LEGALIZECALLACTION_H
#define EZPACKER_LEGALIZECALLACTION_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"
#include "Legalizer/MirLegalizer.h"

namespace LegalizeActions
{
/**
 * This action will pop function calling parameters and will push new instructions right before the call. This is done
 * so other legalization actions can affect parameters.
 *
 * EXAMPLE -> call %dest, i64 %arg1, i64 45
 *
 * RESULT ->
 * PUSH_ARG __bindingToken %token, i64 %arg1
 * PUSH_ARG __bindingToken %token, i64 45
 * call __bindingToken %token, %dest
 */
extern LegalizationResult LegalizeCall(LegalizeCtx &ctx);

}; // namespace LegalizeActions

#endif // EZPACKER_LEGALIZECALLACTION_H
