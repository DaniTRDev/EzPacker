#ifndef EZTARGETS_RISCV64_LOWERING_H
#define EZTARGETS_RISCV64_LOWERING_H

#include "EzTargetsRiscV64Common.h"
#include "Legalizer/Actions/LegalizeActionCommon.h"

/**
 * Declarations of the generated-code contract for the RISC-V 64 lowering shims and rule helpers.
 */
LegalizationResult RiscV64CallLowering(LegalizeCtx &ctx);
LegalizationResult RiscV64ReturnLowering(LegalizeCtx &ctx);
LegalizationResult RiscV64ThrowLowering(LegalizeCtx &ctx);
LegalizationResult RiscV64CatchLowering(LegalizeCtx &ctx);

/// True when val is a positive power of two.
bool isPowTwo(int64_t val);

/// True when val is a strictly positive constant.
bool isPositiveConst(int64_t val);

/**
 * Returns the shift amount for a positive power of two: floor(log2(val)).
 * Returns 0 for non-positive or non-power-of-two values.
 */
int64_t log2Pow2(int64_t val);

/// Returns val - 1, used by power-of-two-minus-one rule transforms.
int64_t sub1(int64_t val);

#endif // EZTARGETS_RISCV64_LOWERING_H
