#ifndef EZTRIPLE_X86_64_LOWERING_H
#define EZTRIPLE_X86_64_LOWERING_H

#include "EzTripleCommon.h"
#include "Legalizer/Actions/LegalizeActionCommon.h"

/**
 * Canonical declaration of the generated-code contract for the hand-written x86-64 lowering shim.
 *
 * The generated legalizer action table and rewrite rules reference these symbols by name:
 *  - the two LOWER entry points registered for CALL/RET, and
 *  - the predicate/transform helpers used by `.lrd` rule `when`/`emit` clauses.
 *
 * Keeping them declared here (rather than only in the generated forward declarations) documents
 * the DSL-visible contract and keeps the definitions from drifting.
 */
LegalizationResult AMD64CallLowering(LegalizeCtx &ctx);
LegalizationResult AMD64ReturnLowering(LegalizeCtx &ctx);

/// True when val is a positive power of two.
bool isPowTwo(int64_t val);

/// True when val is a strictly positive constant.
bool isPositiveConst(int64_t val);

/**
 * Returns the shift amount for a positive power of two: floor(log2(val)), i.e. the number of
 * trailing zero bits. Returns 0 for non-positive or non-power-of-two values. Named so it never
 * shadows `std::log2`.
 */
int64_t log2Pow2(int64_t val);

/// Returns val - 1, used by power-of-two-minus-one rule transforms.
int64_t sub1(int64_t val);

#endif // EZTRIPLE_X86_64_LOWERING_H
