#ifndef EZPACKER_EZTESTTRIPLELEGALIZER_H
#define EZPACKER_EZTESTTRIPLELEGALIZER_H

#include "EzTestTripleCommon.h"

/**
 * ============================================================================
 * LEGALIZATION RULES SPECIFICATION & OVERVIEW (EzTestTriple)
 * ============================================================================
 *
 * The legalizer transforms high-level/generic MIR instructions into target-legal
 * representations according to the following policies:
 *
 * 1. TYPE SIZE & PROMOTION CONSTRAINTS:
 *    - Native legal integer widths are i8, i16, i32, and i64.
 *    - Sub-byte or non-standard types (< i8) are promoted via minSize(idx, i8).
 *    - Native floating-point types are f32 and f64.
 *    - Hardware multiplication, division, and modulo (MUL, IMUL, DIV, IDIV, REM)
 *      are restricted to minimum i16 width (sub-i16 ops are promoted first).
 *
 * 2. EXPANSION BOUNDARIES:
 *    - Any scalar integer ALU, Memory, Shift, or Movement instruction operating
 *      on widths > 64-bit (e.g., i128) triggers `expandIf()` to defer processing
 *      to the MirExpansionRuleRegistry.
 *
 * 3. MEMORY & ALLOCATIONS:
 *    - LOAD / STORE: Native scalar widths (i8-i64, f32, f64, ptr). Types > 64-bit
 *      are expanded into discrete consecutive 64-bit memory accesses.
 *    - ALLOC / DALLOC: Must yield valid Pointer-kind types. Lowering and SP adjustments
 *      are performed during the MirFrameLowerer pass.
 *
 * 4. CONTROL FLOW & SYSTEM:
 *    - Branching (JMP, conditional jumps JE, JNE, JG, etc.), NOP, HALT, and SYSCALL
 *      are inherently legal with no type promotion requirements.
 *
 * 5. CALLING CONVENTION & ABI BINDING:
 *    - CALL / RET: Must bind against the target's internal `BindingToken` type.
 *      If unbound, custom actions (`LegalizeCall` / `LegalizeReturn`) are invoked
 *      to assign physical argument/return registers and layout stack frames.
 *    - PUSH_ARG / POP_ARG / PUSH_RET / POP_RET: Require the first operand to be
 *      the `BindingToken` and expand if payload exceeds 64-bit width.
 * ============================================================================
 */
namespace EzTestTriple
{
/**
 * Creates the legalization rules using the given context and legalizer.
 */
extern void CreateLegalizer(MirBuilderContext *ctx, MirLegalizer *legalizer);
}; // namespace EzTestTriple

#endif // EZPACKER_EZTESTTRIPLELEGALIZER_H