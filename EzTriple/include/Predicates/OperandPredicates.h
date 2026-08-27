#ifndef EZDSL_OPERAND_PREDICATES_H
#define EZDSL_OPERAND_PREDICATES_H

#include "EzTripleCommon.h"

/**
 * Predicates used to know if certain conditions about an operand is true.
 */
namespace Predicates::Operand
{
/**
 * Returns true if the given value is between the given SIGNED limits.
 */
extern bool isInRange(int64_t val, int64_t min, int64_t max);

/**
 * Returns true if the given value is between the given UNSIGNED limits.
 */
extern bool isInRangeU(uint64_t val, uint64_t min, uint64_t max);

/**
 * Returns true if the given value is a power of two.
 */
extern bool isPowerOfTwo(uint64_t val);

/**
 * Returns true if the given operand is an integer and it is negative.
 */
extern bool isIntNegative(MirOperand *operand);

/**
 * Returns true if the given operand is an integer and it is positive (>= 0).
 */
extern bool isIntPositive();

/**
 * Returns true if the given operand is an integer and its value is 0.
 */
extern bool isIntZero(MirOperand *operand);

/**
 * Returns true if the given operand is a floating point value and its value is -0 or +0.
 */
extern bool isFloatZero(MirOperand *operand);

/**
 * Returns true if the given operand is a floating point value and its value is <= -0
 */
extern bool isFloatNegative(MirOPerand *operand);

/**
 * Returns true if the given operand is a floating point value and its value is >= +0
 */
extern bool isFloatPositive(MirOperand *operand);

}; // namespace Predicates::Operand

#endif // EZDSL_OPERAND_PREDICATES_H