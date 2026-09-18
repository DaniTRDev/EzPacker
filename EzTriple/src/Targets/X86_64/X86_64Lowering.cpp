#include "Legalizer/Actions/LegalizeActionCommon.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include <bit>
#include <cstdint>

LegalizationResult AMD64CallLowering(LegalizeCtx &ctx)
{
    return LegalizeActions::LegalizeCall(ctx);
}

LegalizationResult AMD64ReturnLowering(LegalizeCtx &ctx)
{
    return LegalizeActions::LegalizeReturn(ctx);
}

bool isPowTwo(int64_t val)
{
    return val > 0 && (val & (val - 1)) == 0;
}

bool isPositiveConst(int64_t val)
{
    return val > 0;
}

int64_t log2(int64_t val)
{
    if (val <= 0) return 0;
    return std::countr_zero(static_cast<uint64_t>(val));
}

int64_t sub1(int64_t val)
{
    return val - 1;
}
