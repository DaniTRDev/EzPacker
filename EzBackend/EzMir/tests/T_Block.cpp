#include <gtest/gtest.h>
#include "MirTestSuite.h"

class BlockTest : public MirTestSuiteAsGtest
{
  public:
};

TEST_F(BlockTest, AddBlock)
{
    MirBlockBuilder builder(getBuilderCtx(), getTestFunc());
    builder.build(nullptr); // Block 1.
}