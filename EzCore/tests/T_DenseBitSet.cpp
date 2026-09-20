#include "HelperClasses/DenseBitSet.h"
#include <gtest/gtest.h>

/**
 * Verifies basic set/test semantics and that out-of-capacity queries are ignored safely.
 */
TEST(DenseBitSetTest, SetAndTest)
{
    DenseBitSet bits(65);
    bits.set(0);
    bits.set(64);

    EXPECT_TRUE(bits.test(0));
    EXPECT_TRUE(bits.test(64));
    EXPECT_FALSE(bits.test(1));

    // Bits beyond the allocated capacity are silently ignored by set/test.
    bits.set(128);
    EXPECT_FALSE(bits.test(128));
}

/**
 * Regression for WEI-02: computeLiveIn must not index past the shorter operand's word storage.
 * The destination is wider than use/liveOut/def, so the pre-fix implementation read out of bounds.
 */
TEST(DenseBitSetTest, ComputeLiveInWithShorterOperands)
{
    DenseBitSet liveIn(256);
    DenseBitSet use(64);
    DenseBitSet liveOut(64);
    DenseBitSet def(64);

    use.set(3);
    liveOut.set(5);
    def.set(5);

    liveIn.computeLiveIn(use, liveOut, def);

    // LiveIn = Use | (LiveOut & ~Def): bit 3 from Use survives, bit 5 is killed by Def.
    EXPECT_TRUE(liveIn.test(3));
    EXPECT_FALSE(liveIn.test(5));
}

/**
 * Verifies the liveness equation is computed correctly when all bitsets share the same width.
 */
TEST(DenseBitSetTest, ComputeLiveInMatchesTransferFunction)
{
    DenseBitSet liveIn(128);
    DenseBitSet use(128);
    DenseBitSet liveOut(128);
    DenseBitSet def(128);

    use.set(1);
    liveOut.set(1);
    liveOut.set(9);
    def.set(9);

    liveIn.computeLiveIn(use, liveOut, def);

    EXPECT_TRUE(liveIn.test(1));
    EXPECT_FALSE(liveIn.test(9));
}
