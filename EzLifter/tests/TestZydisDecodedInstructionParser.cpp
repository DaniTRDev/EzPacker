#include <gtest/gtest.h>
#include "Decoder/Zydis/EzZydisDecodedOperand.h"
#include "Decoder/Zydis/ZydisDecodedInstructionParser.h"

TEST(ZydisDecodedInstructionParserTests, IsBranchReturnsTrue)
{
    auto instr = std::make_shared<ZydisDecodedInstruction>();
    instr->meta.category = ZYDIS_CATEGORY_COND_BR;

    ZydisDecodedInstructionParser parser(instr);
    EXPECT_TRUE(parser.isBranch());
}

TEST(ZydisDecodedInstructionParserTests, DetectsMemoryUsageCorrectly)
{
    auto operand = std::make_shared<ZydisDecodedOperand>();
    operand->type = ZYDIS_OPERAND_TYPE_MEMORY;

    auto memoryOperand = std::make_shared<EzZydisDecodedOperand>(operand);

    std::vector<std::shared_ptr<IDecodedOperand>> operands = { memoryOperand };
    auto instr = std::make_shared<ZydisDecodedInstruction>();

    ZydisDecodedInstructionParser parser(instr);
    EXPECT_TRUE(parser.usesMemory(operands));
}

