#include "X86_64Lowering.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include <bit>
#include <cstdint>

/// Entry point registered as the AMD64 CALL/lower handler; dispatches to the shared call legalizer.
LegalizationResult AMD64CallLowering(LegalizeCtx &ctx) { return LegalizeActions::LegalizeCall(ctx); }

/// Entry point registered as the AMD64 RET/lower handler; dispatches to the shared return legalizer.
LegalizationResult AMD64ReturnLowering(LegalizeCtx &ctx) { return LegalizeActions::LegalizeReturn(ctx); }

/// Predicate used by legalization rules: true when val is a positive power of two.
bool isPowTwo(int64_t val) { return val > 0 && (val & (val - 1)) == 0; }

/// Predicate used by legalization rules: true when val is a strictly positive constant.
bool isPositiveConst(int64_t val) { return val > 0; }

/// Returns the shift amount for a positive power of two (floor(log2)), or 0 otherwise.
int64_t log2Pow2(int64_t val)
{
    if (val <= 0)
        return 0;
    return std::countr_zero(static_cast<uint64_t>(val));
}

/// Returns val - 1, used by rule transforms that match a power-of-two-minus-one pattern.
int64_t sub1(int64_t val) { return val - 1; }
