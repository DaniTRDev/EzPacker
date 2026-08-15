#ifndef EZPACKER_EZTESTTRIPLEEXPANSIONREGISTRY_H
#define EZPACKER_EZTESTTRIPLEEXPANSIONREGISTRY_H

#include "EzTestTripleCommon.h"

/**
 * ============================================================================
 * EXPANSION REGISTRY RULES OVERVIEW (EzTestTriple)
 * ============================================================================
 *
 * Defines rewrite rules for instructions flagged as oversized (> 64-bit / 128-bit)
 * during legalization. It maps wide 2-operand abstract operations into 64-bit pairs
 * or runtime library ABI calls.
 *
 * 1. DATA MOVEMENT & ABI MARSHALING:
 *    - MOV: Split into independent low (dstLo = srcLo) and high (dstHi = srcHi) moves.
 *    - PUSH_ARG / POP_ARG / PUSH_RET / POP_RET:
 *      Maintains the binding token in dstLo and emits two consecutive tokenized
 *      push/pop entries (one for srcLo/dstLo, one for srcHi/dstHi).
 *
 * 2. MEMORY LOAD / STORE:
 *    - LOAD:  Emit 64-bit load at [base + 0] into dstLo, followed by 64-bit load
 *             at [base + 8] into dstHi.
 *    - STORE: Emit 64-bit store from srcLo into [base + 0], followed by 64-bit
 *             store from srcHi into [base + 8].
 *
 * 3. MULTI-PRECISION ALU & CARRY CHAINS:
 *    - ADD:  ADD dstLo, srcLo (updates carry flag) -> ADC dstHi, srcHi (consumes carry).
 *    - ADC:  ADC dstLo, srcLo -> ADC dstHi, srcHi (continuation carry chain).
 *    - SUB:  SUB dstLo, srcLo (updates borrow flag) -> SBB dstHi, srcHi (consumes borrow).
 *    - SBB:  SBB dstLo, srcLo -> SBB dstHi, srcHi (continuation borrow chain).
 *    - NEG:  NOT dstLo; NOT dstHi; ADD dstLo, 1; ADC dstHi, 0 (Two's complement negation).
 *    - AND / OR / XOR / NOT: Bitwise slice operations evaluated independently on Lo/Hi chunks.
 *
 * 4. COMPLEX MATH & WIDE SHIFTS (Runtime Library Lowering):
 *    - Multiplication: MUL / IMUL  --> Call "__multi3" (R0:R2 = Low:High inputs/outputs).
 *    - Division:       DIV         --> Call "__udivti3"
 *                      IDIV        --> Call "__divti3"
 *    - Modulo:         REM         --> Call "__umodti3"
 *    - Shifts:         SHL         --> Call "__ashlti3"
 *                      SHR         --> Call "__lshrti3"
 *                      SAR         --> Call "__ashrti3"
 *
 * 5. COMPARISONS & CASTS:
 *    - CMP: Emulates a non-destructive multi-word subtract using temporal registers
 *           (TempLo(0) = dstLo - srcLo; TempHi(0) = dstHi - srcHi - borrow).
 *    - TRUNC:   Extracts dstLo from srcLo.
 *    - ZEXT:    Copies srcLo into dstLo, sets dstHi to 0.
 *    - SEXT:    Copies srcLo into dstLo, sets dstHi to srcLo, then arithmetic shifts (SAR) dstHi by 63.
 *    - BITCAST: Direct copy across low/high physical sub-registers.
 * ============================================================================
 */
namespace EzTestTriple
{
/**
 * Creates the expansion registry.
 */
extern void CreateExpansionRegistry(MirExpansionRuleRegistry *registry);
}; // namespace EzTestTriple

#endif // EZPACKER_EZTESTTRIPLEEXPANSIONREGISTRY_H