#ifndef EZPACKER_EZTESTTRIPLEREGISTERBANKS_H
#define EZPACKER_EZTESTTRIPLEREGISTERBANKS_H

#include "EzTestTripleCommon.h"

/**
 * ============================================================================
 * Register Banks Overview & ABI Conventions
 * ============================================================================
 *
 * ----------------------------------------------------------------------------
 * Register Bank 0: GPR (General Purpose Registers)
 * Classes: GPR64 (native), GPR32 (sub), GPR16 (sub), GPR8 (sub)
 * ----------------------------------------------------------------------------
 *  ID | GPR64 | GPR32 | GPR16 | GPR8 | Role / ABI Convention
 * ----+-------+-------+-------+------+----------------------------------------
 *   0 | R0    | R0D   | R0W   | R0B  | Return Value 0 / Caller-saved
 *   1 | R1    | R1D   | R1W   | R1B  | Argument 3     / Caller-saved
 *   2 | R2    | R2D   | R2W   | R2B  | Argument 2     / Return Value 1
 *   3 | R3    | R3D   | R3W   | R3B  | General Scratch / Callee-saved
 *   4 | RSP   | -     | -     | -    | Stack Pointer (SP)
 *   5 | RFP   | -     | -     | -    | Frame Pointer (FP) / Callee-saved
 *   6 | R6    | R6D   | R6W   | R6B  | Argument 1     / Caller-saved
 *   7 | R7    | R7D   | R7W   | R7B  | Argument 0     / Caller-saved
 *   8 | R8    | R8D   | R8W   | R8B  | Argument 4     / Caller-saved
 *   9 | R9    | R9D   | R9W   | R9B  | Argument 5     / Caller-saved
 *  10 | R10   | R10D  | R10W  | R10B | Scratch        / Caller-saved
 *  11 | R11   | R11D  | R11W  | R11B | Scratch        / Caller-saved
 *  12 | R12   | R12D  | R12W  | R12B | General        / Callee-saved
 *  13 | R13   | R13D  | R13W  | R13B | General        / Callee-saved
 *  14 | R14   | R14D  | R14W  | R14B | General        / Callee-saved
 *  15 | R15   | R15D  | R15W  | R15B | General        / Callee-saved
 *
 * ----------------------------------------------------------------------------
 * Register Bank 1: FPR (Floating Point Registers)
 * Classes: FPR64 (native f64), FPR32 (sub f32)
 * ----------------------------------------------------------------------------
 *  ID | FPR64 | FPR32  | Role / ABI Convention
 * ----+-------+--------+------------------------------------------------------
 *   0 | XMM0  | XMM0_S | Float Arg 0 / Float Return 0 / Caller-saved
 *   1 | XMM1  | XMM1_S | Float Arg 1 / Caller-saved
 *   2 | XMM2  | XMM2_S | Float Arg 2 / Caller-saved
 *   3 | XMM3  | XMM3_S | Float Arg 3 / Caller-saved
 *   4 | XMM4  | XMM4_S | Float Scratch / Caller-saved
 *   5 | XMM5  | XMM5_S | Float Scratch / Caller-saved
 *   6 | XMM6  | XMM6_S | Float Scratch / Caller-saved
 *   7 | XMM7  | XMM7_S | Float Scratch / Caller-saved
 *
 * ----------------------------------------------------------------------------
 * Register Bank 2: SPR (Special Registers)
 * Classes: SPR64
 * ----------------------------------------------------------------------------
 *  ID | SPR64 | Role
 * ----+-------+--------+------------------------------------------------------
 *  0  | RIP  | Instruction pointer
 */
namespace EzTestTriple
{
namespace RegisterIds
{
// GPR IDs
constexpr size_t R0 = 0;  // Ret0
constexpr size_t R1 = 1;  // Arg3
constexpr size_t R2 = 2;  // Arg2 / Ret1
constexpr size_t R3 = 3;  // Callee-saved
constexpr size_t RSP = 4; // Stack Pointer
constexpr size_t RFP = 5; // Frame Pointer
constexpr size_t R6 = 6;  // Arg1
constexpr size_t R7 = 7;  // Arg0
constexpr size_t R8 = 8;  // Arg4
constexpr size_t R9 = 9;  // Arg5
constexpr size_t R10 = 10;
constexpr size_t R11 = 11;
constexpr size_t R12 = 12; // Callee-saved
constexpr size_t R13 = 13; // Callee-saved
constexpr size_t R14 = 14; // Callee-saved
constexpr size_t R15 = 15; // Callee-saved

// FPR IDs
constexpr size_t XMM0 = 0; // Float Arg0 / Ret0
constexpr size_t XMM1 = 1; // Float Arg1
constexpr size_t XMM2 = 2; // Float Arg2
constexpr size_t XMM3 = 3; // Float Arg3
constexpr size_t XMM4 = 4;
constexpr size_t XMM5 = 5;
constexpr size_t XMM6 = 6;
constexpr size_t XMM7 = 7;

// SPECIAL
constexpr size_t RIP = 0;
}; // namespace RegisterIds

namespace Banks
{
extern MirRegisterBank *GPR;
extern MirRegisterBank *FPR;
extern MirRegisterBank *SPR;
}; // namespace Banks

/**
 * Creates the banks of registers for EzTestTriple.
 */
extern std::pmr::vector<MirRegisterBank *> CreateRegisterBanks(std::pmr::memory_resource *alloc);
}; // namespace EzTestTriple

#endif // EZPACKER_EZTESTTRIPLEREGISTERBANKS_H