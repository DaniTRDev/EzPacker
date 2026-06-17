#include <gtest/gtest.h>
#include "EzTripleTestSuite/EzTripleTestSuite.h"

class PromoteScalarActionTest : public MirTestSuiteAsGtest
{
  public:
  
};

TEST_F(PromoteScalarActionTest, SimpleArithPromotion)
{
    const auto &t = getTypeTable();
    MirInstructionBuilder instrBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());
    
    MirRegister *op1 = opBuilder.buildVReg(t->i8(), "testReg");
    MirRegister *op2 = opBuilder.buildVReg(t->i32(), "testReg");
    instrBuilder.ADD(op1, op2);
    
    
}