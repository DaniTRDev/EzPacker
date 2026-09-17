#include "EzDslLexerTestSuite.h"
#include "Ast/TargetInstDefLangAst.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetInstDefLang.h"

class TargetInstDefLangTest : public DslLexerTestSuiteAsGtest
{
};

TEST_F(TargetInstDefLangTest, TestTargetOperandIn)
{
    std::string test = "GPR32:src IN";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetOperand, DSL::Ast::TargetInstDef::TargetOperandDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_regClassOrType.m_node, "GPR32");
    EXPECT_EQ(res->m_name.m_node, "src");
    EXPECT_EQ(res->m_direction, DSL::Ast::TargetInstDef::OperandDirection::In);
}

TEST_F(TargetInstDefLangTest, TestTargetOperandOut)
{
    std::string test = "GPR64:dst OUT";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetOperand, DSL::Ast::TargetInstDef::TargetOperandDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_regClassOrType.m_node, "GPR64");
    EXPECT_EQ(res->m_name.m_node, "dst");
    EXPECT_EQ(res->m_direction, DSL::Ast::TargetInstDef::OperandDirection::Out);
}

TEST_F(TargetInstDefLangTest, TestTargetOperandInOut)
{
    std::string test = "Mem32:addr INOUT";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetOperand, DSL::Ast::TargetInstDef::TargetOperandDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_regClassOrType.m_node, "Mem32");
    EXPECT_EQ(res->m_name.m_node, "addr");
    EXPECT_EQ(res->m_direction, DSL::Ast::TargetInstDef::OperandDirection::InOut);
}

TEST_F(TargetInstDefLangTest, TestTargetInstructionDeclaration)
{
    std::string test = R"(
        target_inst ADD32rr(GPR32:dst OUT, GPR32:src1 IN, GPR32:src2 IN) {
            MNEMONIC("addl");
            FLAGS(IsCommutative);
            IMPLICIT_DEFS(EFLAGS);
            IMPLICIT_USES(EAX, EDX);
        };
    )";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetInstDecl, DSL::Ast::TargetInstDef::TargetInstDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_instName.m_node, "ADD32rr");
    ASSERT_EQ(res->m_operands.size(), 3);
    EXPECT_EQ(res->m_operands[0].m_name.m_node, "dst");
    EXPECT_EQ(res->m_operands[0].m_direction, DSL::Ast::TargetInstDef::OperandDirection::Out);
    EXPECT_EQ(res->m_operands[1].m_name.m_node, "src1");
    EXPECT_EQ(res->m_operands[2].m_name.m_node, "src2");

    ASSERT_TRUE(res->m_mnemonic.has_value());
    EXPECT_EQ(res->m_mnemonic->m_node, "addl");

    ASSERT_EQ(res->m_flags.size(), 1);
    EXPECT_EQ(res->m_flags[0].m_node, "IsCommutative");

    ASSERT_EQ(res->m_implicitDefs.size(), 1);
    EXPECT_EQ(res->m_implicitDefs[0].m_node, "EFLAGS");

    ASSERT_EQ(res->m_implicitUses.size(), 2);
    EXPECT_EQ(res->m_implicitUses[0].m_node, "EAX");
    EXPECT_EQ(res->m_implicitUses[1].m_node, "EDX");
}

TEST_F(TargetInstDefLangTest, TestTargetInstructionFile)
{
    std::string test = R"(
        target AMD64;

        target_inst NOP() {
            MNEMONIC("nop");
        };

        target_inst ADD32rm(GPR32:dst OUT, GPR32:src IN, Mem32:addr IN) {
            MNEMONIC("addl");
            FLAGS(ReadsMemory);
            IMPLICIT_DEFS(EFLAGS);
        };
    )";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetInstFile, DSL::Ast::TargetInstDef::TargetInstFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(res->m_targetName.has_value());
    EXPECT_EQ(res->m_targetName->m_node, "AMD64");

    ASSERT_EQ(res->m_instructions.size(), 2);
    EXPECT_EQ(res->m_instructions[0].m_instName.m_node, "NOP");
    EXPECT_EQ(res->m_instructions[0].m_operands.size(), 0);

    EXPECT_EQ(res->m_instructions[1].m_instName.m_node, "ADD32rm");
    EXPECT_EQ(res->m_instructions[1].m_operands.size(), 3);
}
