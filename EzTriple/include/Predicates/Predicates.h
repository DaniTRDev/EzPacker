#ifndef EZTRIPLE_PREDICATES_H
#define EZTRIPLE_PREDICATES_H

#include "EzTripleCommon.h"

enum class MirInstructionOpCode : uint16_t;

// Forward declarations
class MirOperand;
class MirInstruction;
class MirFunctionRegisterInfo;

namespace Predicates
{

/**
 * Checks if a virtual register operand is used exactly once across the function.
 */
extern bool hasOneUse(const MirOperand *operand, const MirFunctionRegisterInfo *regInfo);

/**
 * Retrieves the defining instruction for a given register operand.
 */
extern MirInstruction *getDefiningInstr(const MirOperand *operand, const MirFunctionRegisterInfo *regInfo);

/**
 * Checks if the operand was produced by an instruction with the given opcode.
 */
extern bool
isDefinedByOpcode(const MirOperand *operand, MirInstructionOpCode opcode, const MirFunctionRegisterInfo *regInfo);

/**
 * Checks if the instruction defining this operand is pure (has no side effects,
 * does not write/read memory, does not branch/call) and can be safely folded.
 */
extern bool canFoldWithoutSideEffects(const MirOperand *operand, const MirFunctionRegisterInfo *regInfo);

/**
 * Checks if a signed integer immediate fits in [min, max].
 */
extern bool isInRange(int64_t val, int64_t min, int64_t max);

/**
 * Checks if an unsigned integer immediate fits in [min, max].
 */
extern bool isInRangeU(uint64_t val, uint64_t min, uint64_t max);

/**
 * Checks if a shift amount is valid for the target bit width: [0, bitWidth - 1].
 */
extern bool isValidShiftAmount(const MirOperand *operand, uint32_t bitWidth);

/**
 * Checks if a memory displacement immediate is aligned to byteAlignment.
 */
extern bool isAlignedDisplacement(const MirOperand *disp, uint32_t byteAlignment);

/**
 * Checks if an unsigned integer is an exact power of two (val = 2^k, val > 0).
 */
extern bool isPowerOfTwo(uint64_t val);

/**
 * Checks if val = 2^k + 1 (e.g., 3, 5, 9, 17, 33).
 */
extern bool isPowerOfTwoPlusOne(uint64_t val);

/**
 * Checks if val = 2^k - 1 (e.g., 1, 3, 7, 15, 31, 63).
 */
extern bool isPowerOfTwoMinusOne(uint64_t val);

/**
 * Checks if the operand is an integer constant with all active bits set to 1 (~0).
 */
extern bool isAllOnes(const MirOperand *operand);

/**
 * Checks if the bits in val form a contiguous run of 1s (e.g., 0b00111100).
 * Extracts the trailing zero offset and length of the mask.
 */
extern bool isContiguousMask(uint64_t val, uint32_t &maskOffset, uint32_t &maskLength);

/**
 * Checks if an integer constant is 0.
 */
extern bool isIntZero(const MirOperand *operand);

/**
 * Checks if an integer constant is strictly negative (< 0).
 */
extern bool isIntNegative(const MirOperand *operand);

/**
 * Checks if an integer constant is non-negative (>= 0).
 */
extern bool isIntPositive(const MirOperand *operand);

/**
 * Checks if the operand's high bits above bitWidth are guaranteed to be zero.
 */
extern bool
isZeroExtendedFrom(const MirOperand *operand, uint32_t bitWidth, const MirFunctionRegisterInfo *regInfo = nullptr);

/**
 * Checks if the operand's high bits above bitWidth are sign copies of the bitWidth-th bit.
 */
extern bool
isSignExtendedFrom(const MirOperand *operand, uint32_t bitWidth, const MirFunctionRegisterInfo *regInfo = nullptr);

/**
 * Checks if only the low liveBits are consumed by all users.
 */
extern bool
areHighBitsIgnored(const MirOperand *operand, uint32_t liveBits, const MirFunctionRegisterInfo *regInfo = nullptr);

/**
 * Checks if a float constant is +0.0 or -0.0.
 */
extern bool isFloatZero(const MirOperand *operand);

/**
 * Checks if a float constant is negative (< 0.0 or -0.0).
 */
extern bool isFloatNegative(const MirOperand *operand);

/**
 * Checks if a float constant is positive (> 0.0 or +0.0).
 */
extern bool isFloatPositive(const MirOperand *operand);

/**
 * Checks if a scale factor is valid for hardware indexed addressing (1, 2, 4, 8).
 */
extern bool isValidScaleFactor(int64_t scale);

/**
 * Checks if an operand represents an abstract StackFrameObject reference.
 */
extern bool isFrameIndex(const MirOperand *operand);

}; // namespace Predicates

#endif // EZTRIPLE_PREDICATES_H