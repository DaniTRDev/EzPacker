#ifndef EZTRIPLE_LEGALIZE_LIBCALL_ACTION_H
#define EZTRIPLE_LEGALIZE_LIBCALL_ACTION_H

#include "Legalizer/Actions/LegalizeActionCommon.h"
#include <string_view>

namespace LegalizeActions
{

/**
 * Legalizes an operation by rewriting it into a runtime function call (libcall)
 * using the specified symbol name and standard target calling convention.
 */
LegalizationResult LegalizeLibcall(LegalizeCtx &ctx, std::string_view libcallSymbol);

} // namespace LegalizeActions

#endif // EZTRIPLE_LEGALIZE_LIBCALL_ACTION_H
