#ifndef EZTRIPLE_LEGALIZE_LIBCALL_ACTION_H
#define EZTRIPLE_LEGALIZE_LIBCALL_ACTION_H

#include "Legalizer/Actions/LegalizeActionCommon.h"
#include "Libcall/LibcallKind.h"
#include <string_view>

class CallingConvDesc;

namespace LegalizeActions
{

/**
 * Legalizes an operation by rewriting it into a runtime function call (libcall)
 * using the specified symbol name and optional calling convention override.
 */
LegalizationResult LegalizeLibcall(LegalizeCtx &ctx,
                                   std::string_view libcallSymbol,
                                   CallingConvDesc *overrideCC = nullptr);

/**
 * Legalizes an operation by rewriting it into the canonical runtime function call
 * designated by LibcallKind, consulting the target descriptor's libcall registry
 * for naming, calling convention, and availability.
 */
LegalizationResult LegalizeLibcall(LegalizeCtx &ctx, LibcallKind kind);

} // namespace LegalizeActions

#endif // EZTRIPLE_LEGALIZE_LIBCALL_ACTION_H
