#include "FlexNumber/FlexFloat.h"
#include "FlexNumber/FlexInt.h"

#include <gtest/gtest.h>
#include <cmath>

/**
 * Regression coverage for the FlexInt/FlexFloat consolidation work (DUP-01..05, OPT-05/07):
 * all four integral constructors, cross-width equality, half splitting and float comparisons.
 */
TEST(FlexIntTest, IntegerConstructorsPreserveValueAndWidth)
{
    EXPECT_EQ(FlexInt(uint32_t(4000000000u), 32).getU64(), 4000000000ULL);
    EXPECT_EQ(FlexInt(int32_t(-5), 32).getI64(), -5);
    EXPECT_EQ(FlexInt(uint64_t(123456789012345ULL), 64).getU64(), 123456789012345ULL);
    EXPECT_EQ(FlexInt(int64_t(-9876543210LL), 64).getI64(), -9876543210LL);

    // Default widths are 32 for the 32-bit overloads and 64 for the 64-bit overloads.
    EXPECT_EQ(FlexInt(uint32_t(1)).getBitSize(), 32u);
    EXPECT_EQ(FlexInt(uint64_t(1)).getBitSize(), 64u);
    EXPECT_EQ(FlexInt(int32_t(1)).getBitSize(), 32u);
    EXPECT_EQ(FlexInt(int64_t(1)).getBitSize(), 64u);

    EXPECT_TRUE(FlexInt(uint32_t(1)).isSigned() == false);
    EXPECT_TRUE(FlexInt(int32_t(1)).isSigned() == true);
}

TEST(FlexIntTest, ArithmeticOperators)
{
    EXPECT_EQ((FlexInt(int32_t(7), 32) + FlexInt(int32_t(5), 32)).getI64(), 12);
    EXPECT_EQ((FlexInt(int32_t(10), 32) - FlexInt(int32_t(3), 32)).getI64(), 7);
    EXPECT_EQ((FlexInt(int32_t(6), 32) * FlexInt(int32_t(7), 32)).getI64(), 42);
    EXPECT_EQ((FlexInt(int32_t(20), 32) / FlexInt(int32_t(6), 32)).getI64(), 3);
    EXPECT_EQ((FlexInt(int32_t(20), 32) % FlexInt(int32_t(6), 32)).getI64(), 2);
}

TEST(FlexIntTest, CrossWidthEquality)
{
    // Same sign, differing widths: libtommath stores sign+magnitude so no sign extension copy is needed.
    EXPECT_TRUE(FlexInt(int32_t(-1), 8) == FlexInt(int32_t(-1), 16));
    EXPECT_TRUE(FlexInt(int32_t(-42), 8) == FlexInt(int32_t(-42), 16));
    EXPECT_FALSE(FlexInt(int32_t(-2), 8) == FlexInt(int32_t(-1), 16));
    EXPECT_TRUE(FlexInt(uint32_t(5), 8) == FlexInt(uint32_t(5), 16));

    // Differing signedness is never equal, even when the two's-complement bit patterns match.
    EXPECT_FALSE(FlexInt(int32_t(-1), 8) == FlexInt(uint32_t(255), 8));
}

TEST(FlexIntTest, HalfSplitting)
{
    FlexInt value(uint64_t(0x12345678ULL), 32);
    FlexInt high = value.getHighHalf();
    FlexInt low = value.getLowHalf();
    EXPECT_EQ(high.getU64(), 0x1234ULL);
    EXPECT_EQ(low.getU64(), 0x5678ULL);

    // Odd widths cannot be split.
    FlexInt odd(uint32_t(3), 7);
    EXPECT_THROW(odd.getHighHalf(), std::invalid_argument);
    EXPECT_THROW(odd.getLowHalf(), std::invalid_argument);
}

TEST(FlexFloatTest, ComparisonNaNSemantics)
{
    FlexFloat one(1.0);
    FlexFloat two(2.0);

    EXPECT_TRUE(one < two);
    EXPECT_TRUE(one <= one);
    EXPECT_TRUE(two > one);
    EXPECT_TRUE(one == one);
    EXPECT_TRUE(one != two);

    FlexFloat nan(std::nan(""));
    EXPECT_FALSE(nan == nan);
    EXPECT_TRUE(nan != nan);
    EXPECT_FALSE(nan < one);
    EXPECT_FALSE(nan > one);
    EXPECT_FALSE(nan <= one);
    EXPECT_FALSE(nan >= one);
}

TEST(FlexFloatTest, Arithmetic)
{
    FlexFloat sum = FlexFloat(1.5) + FlexFloat(2.25);
    EXPECT_DOUBLE_EQ(sum.getDouble(), 3.75);

    FlexFloat difference = FlexFloat(5.0) - FlexFloat(1.25);
    EXPECT_DOUBLE_EQ(difference.getDouble(), 3.75);

    FlexFloat product = FlexFloat(3.0) * FlexFloat(2.5);
    EXPECT_DOUBLE_EQ(product.getDouble(), 7.5);

    FlexFloat quotient = FlexFloat(7.5) / FlexFloat(2.5);
    EXPECT_DOUBLE_EQ(quotient.getDouble(), 3.0);

    EXPECT_THROW(FlexFloat(1.0) / FlexFloat(0.0), std::domain_error);
}
