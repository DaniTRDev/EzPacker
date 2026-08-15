#ifndef EZPACKER_EZTESTTRIPLEINSTRUCTIONSELECTOR_H
#define EZPACKER_EZTESTTRIPLEINSTRUCTIONSELECTOR_H

#include "EzTestTripleCommon.h"
#include "EzTestTripleInstructionSet.h"
#include "RegisterBanks/EzTestTripleRegisterBanks.h"

/**
 * ============================================================================
 * INSTRUCTION SELECTION RULES SPECIFICATION & OVERVIEW (EzTestTriple)
 * ============================================================================
 *
 * The Instruction Selector maps legalized MIR abstract operations and virtual
 * operands directly to target-specific hardware instructions.
 *
 * 1. PREDICATE MATCHING STRATEGY:
 *    - Uses operand type analysis (ISelPreds::operandMirType) and MIR opcodes.
 *    - Follows a 2-operand machine model (Destination = Operand 0 / First Operand).
 *    - Memory operands match load/store patterns directly to register-memory (rm/mr)
 *      machine instructions.
 *
 * 2. DATA MOVEMENT & MEMORY MAPPING:
 *    - MOV:  Matches integer bitwidths (8, 16, 32, 64), pointers (64-bit),
 *            and IEEE-754 floating point sizes (f32 -> MOVSS, f64 -> MOVSD).
 *    - LOAD: Maps to MOV[8|16|32|64]rm and scalar float moves MOVSSrm / MOVSDrm.
 *    - STORE: Maps to MOV[8|16|32|64]mr and scalar float stores MOVSSmr / MOVSDmr.
 *
 * 3. ARITHMETIC & LOGIC (ALU):
 *    - Sized integer operations (ADD, SUB, AND, OR, XOR, CMP, TEST) map directly
 *      to their matching register-register machine opcodes (e.g., ADD32rr, ADD64rr).
 *    - Carry/Borrow ALU ops (ADC, SBB) map to ADC32rr/ADC64rr and SBB32rr/SBB64rr.
 *    - Hardware Mul/Div: Maps IMUL, IDIV, and DIV to 32-bit and 64-bit hardware forms.
 *    - Floating Point: Maps FADD, FSUB, FMUL, FDIV, and FCMP to 32-bit (SS) and
 *      64-bit (SD) scalar execution units.
 *
 * 4. CASTING, EXTENSIONS & SHIFTS:
 *    - ZEXT / SEXT: Maps to zero-extend (MOVZX) and sign-extend (MOVSX) forms.
 *    - TRUNC: Maps to sub-register truncation moves (TRUNC8rr, TRUNC16rr, TRUNC32rr).
 *    - Float/Int Conversions: Maps scalar conversion primitives
 *      (CVTSS2SD, CVTSD2SS, CVTSI2SS, CVTSI2SD, CVTSS2SI, CVTSD2SI).
 *    - Shifts: Maps SHL, SHR, SAR to register-shift instructions (SHL32rr, etc.).
 *
 * 5. CONTROL FLOW, STACK & SYSTEM INSTRUCTIONS:
 *    - Unconditional (JMP) and Conditional branches (JE, JNE, JG, JGE, JL, JLE, JA, JB)
 *      map directly via catch-all predicates.
 *    - PUSH / POP: Matched on 64-bit operands to manage register spills / callee saves.
 *    - System: CALL, RET, NOP, HLT, and SYSCALL pass through directly to machine ops.
 *
 * 6. PSEUDO INSTRUCTIONS:
 *  - DYNAMIC_STACKALLOC (for DALLOC) and ALLOC (for ALLOC). This instructions are here just to preserve
 *  the rule "no High-Level/PassInternal instructions after selection pass".
 * ============================================================================
 */
namespace EzTestTriple
{
/**
 * Creates the selection rules with the given ctx and selector.
 */
extern void CreateInstructionSelector(MirBuilderContext *ctx, MirInstructionSelector *selector);
}; // namespace EzTestTriple

#endif