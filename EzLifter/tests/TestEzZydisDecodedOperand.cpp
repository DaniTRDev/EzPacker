#include <gtest/gtest.h>
#include "Decoder/Zydis/EzZydisDecodedOperand.h"

TEST(EzZydisDecodedOperandTests, GetImmediateSReturnsCorrectValue)
{
    auto operand = std::make_shared<ZydisDecodedOperand>();
    operand->type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    operand->imm.value.s = -12345;

    EzZydisDecodedOperand decoded(operand);
    EXPECT_TRUE(decoded.isImmediate());
    EXPECT_EQ(decoded.getImmediateS(), static_cast<uint64_t>(-12345));
}

TEST(EzZydisDecodedOperandTests, GetRegisterReturnsCorrectId)
{
    auto operand = std::make_shared<ZydisDecodedOperand>();
    operand->type = ZYDIS_OPERAND_TYPE_REGISTER;
    operand->reg.value = ZYDIS_REGISTER_EAX;

    EzZydisDecodedOperand decoded(operand);
    EXPECT_TRUE(decoded.isRegister());
    EXPECT_EQ(decoded.getRegister(), static_cast<int16_t>(ZYDIS_REGISTER_EAX));
}

TEST(EzZydisDecodedOperandTests, HasDisplacementWorks)
{
    auto operand = std::make_shared<ZydisDecodedOperand>();
    operand->type = ZYDIS_OPERAND_TYPE_MEMORY;
    operand->mem.disp.has_displacement = ZYAN_TRUE;

    EzZydisDecodedOperand decoded(operand);
    EXPECT_TRUE(decoded.hasDisplacement());
}
