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

TEST(FlexIntTest, ModularWrapAround)
{
    // 32-bit wrap around
    FlexInt maxU32(uint32_t(0xFFFFFFFF), 32);
    FlexInt oneU32(uint32_t(1), 32);
    FlexInt wrapped = maxU32 + oneU32;
    EXPECT_EQ(wrapped.getU64(), 0ULL);

    // 128-bit modular addition
    FlexInt val128_max("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 128, false, 16);
    FlexInt one128(uint64_t(1), 128);
    FlexInt wrapped128 = val128_max + one128;
    EXPECT_TRUE(wrapped128.isZero());
    EXPECT_EQ(wrapped128.toString(16), "0");

    // 256-bit modular wrap
    FlexInt max256("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 256, false, 16);
    FlexInt one256(uint64_t(1), 256);
    FlexInt wrapped256 = max256 + one256;
    EXPECT_TRUE(wrapped256.isZero());
}

TEST(FlexIntTest, CheckedArithmetic)
{
    FlexInt a(uint32_t(0xFFFFFFFF), 32);
    FlexInt b(uint32_t(1), 32);
    FlexInt res(uint32_t(0), 32);
    bool overflowed = a.addWithOverflow(b, res);
    EXPECT_TRUE(overflowed);
    EXPECT_EQ(res.getU64(), 0ULL);

    FlexInt c(uint32_t(10), 32);
    FlexInt d(uint32_t(20), 32);
    overflowed = c.addWithOverflow(d, res);
    EXPECT_FALSE(overflowed);
    EXPECT_EQ(res.getU64(), 30ULL);
}

TEST(FlexIntTest, BitwiseOperators)
{
    FlexInt x(uint64_t(0x0F0F0F0F0F0F0F0FULL), 64);
    FlexInt y(uint64_t(0xF0F0F0F0F0F0F0F0ULL), 64);
    EXPECT_EQ((x & y).getU64(), 0ULL);
    EXPECT_EQ((x | y).getU64(), 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ((x ^ y).getU64(), 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ((~x).getU64(), 0xF0F0F0F0F0F0F0F0ULL);

    // 128-bit bitwise
    FlexInt x128("0x0000000000000000FFFFFFFFFFFFFFFF", 128, false, 16);
    FlexInt y128("0xFFFFFFFFFFFFFFFF0000000000000000", 128, false, 16);
    EXPECT_TRUE((x128 & y128).isZero());
    EXPECT_EQ((x128 | y128).toString(16), "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    EXPECT_EQ((~x128).toString(16), "FFFFFFFFFFFFFFFF0000000000000000");

    FlexInt compound(uint64_t(0x0F), 8);
    compound |= FlexInt(uint64_t(0xF0), 8);
    EXPECT_EQ(compound.getU64(), 0xFFULL);
}

TEST(FlexIntTest, Shifts)
{
    FlexInt v64(uint64_t(1), 64);
    EXPECT_EQ((v64.shl(10)).getU64(), 1024ULL);
    EXPECT_EQ((v64.shl(64)).getU64(), 0ULL);

    FlexInt v128(uint64_t(1), 128);
    FlexInt shifted128 = v128.shl(64);
    EXPECT_EQ(shifted128.extractWord64(1), 1ULL);
    EXPECT_EQ(shifted128.extractWord64(0), 0ULL);

    FlexInt rshifted = shifted128.lshr(64);
    EXPECT_EQ(rshifted.extractWord64(0), 1ULL);
    EXPECT_EQ(rshifted.extractWord64(1), 0ULL);

    FlexInt sneg(int32_t(-4), 32);
    EXPECT_EQ((sneg.ashr(1)).getI64(), -2);
    EXPECT_EQ((sneg.ashr(32)).getI64(), -1);
}

TEST(FlexIntTest, ArbitrarySlicingAndLimbReconstruction)
{
    std::vector<uint64_t> limbs = { 0x1122334455667788ULL, 0x99AABBCCDDEEFF00ULL };
    FlexInt reconstructed = FlexInt::fromLimbs64(limbs, 128, false);
    EXPECT_EQ(reconstructed.extractWord64(0), 0x1122334455667788ULL);
    EXPECT_EQ(reconstructed.extractWord64(1), 0x99AABBCCDDEEFF00ULL);
    EXPECT_EQ(reconstructed.extractBits(32, 32, false).getU64(), 0x11223344ULL);
}

TEST(FlexIntTest, ByteSerializationAndEndianness)
{
    FlexInt big("0x0102030405060708090A0B0C0D0E0F10", 128, false, 16);
    std::vector<uint8_t> leBytes(16);
    big.writeBytes(leBytes, Endianness::Little);
    EXPECT_EQ(leBytes[0], 0x10);
    EXPECT_EQ(leBytes[15], 0x01);

    std::vector<uint8_t> beBytes(16);
    big.writeBytes(beBytes, Endianness::Big);
    EXPECT_EQ(beBytes[0], 0x01);
    EXPECT_EQ(beBytes[15], 0x10);

    FlexInt fromLe = FlexInt::readBytes(leBytes, 128, false, Endianness::Little);
    EXPECT_EQ(fromLe.toString(16), "102030405060708090A0B0C0D0E0F10");

    FlexInt fromBe = FlexInt::readBytes(beBytes, 128, false, Endianness::Big);
    EXPECT_EQ(fromBe.toString(16), "102030405060708090A0B0C0D0E0F10");
}

TEST(FlexFloatTest, IeeeInterchangeEncoding)
{
    // f32: 1.0 -> 0x3F800000
    FlexFloat f1(1.0f);
    FlexInt bitsF32 = f1.bitcastToFlexInt();
    EXPECT_EQ(bitsF32.getU64(), 0x3F800000ULL);

    // f64: 1.0 -> 0x3FF0000000000000
    FlexFloat d1(1.0);
    FlexInt bitsF64 = d1.bitcastToFlexInt();
    EXPECT_EQ(bitsF64.getU64(), 0x3FF0000000000000ULL);

    // Roundtrip bitcast f64
    FlexFloat roundtrip = FlexFloat::bitcastFromFlexInt(bitsF64, 64);
    EXPECT_DOUBLE_EQ(roundtrip.getDouble(), 1.0);

    // f128: 1.0 -> 0x3FFF0000000000000000000000000000
    FlexFloat quad("1.0", 128, 10);
    FlexInt bitsF128 = quad.bitcastToFlexInt();
    EXPECT_EQ(bitsF128.extractWord64(1), 0x3FFF000000000000ULL);
    EXPECT_EQ(bitsF128.extractWord64(0), 0ULL);

    // Roundtrip bitcast f128
    FlexFloat quadBack = FlexFloat::bitcastFromFlexInt(bitsF128, 128);
    EXPECT_DOUBLE_EQ(quadBack.getDouble(), 1.0);

    // Byte serialization of f64
    std::vector<uint8_t> fBytes(8);
    d1.writeIeeeBytes(fBytes, Endianness::Little);
    FlexFloat readFloat = FlexFloat::readIeeeBytes(fBytes, 64, Endianness::Little);
    EXPECT_DOUBLE_EQ(readFloat.getDouble(), 1.0);
}

