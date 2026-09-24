#include "EzDslLexerTestSuite.h"
#include "Ast/InstructionSelectDefLangAst.h"
#include "Parser/InstructionSelectDefLang.h"
#include "Parser/ParseContext.h"

/**
 * Test fixture for the instruction selection pattern (.isd) dialect parser.
 */
class InstructionSelectDefLangTest : public DslLexerTestSuiteAsGtest
{
};

/**
 * Verifies an addressing mode declaration parses its typed parameters (with
 * default value) and match/when variants.
 */
TEST_F(InstructionSelectDefLangTest, TestAddrModeDeclaration)
{
    std::string test = R"(
        addrmode AddrModeRegImm(GPR64:base, simm32:disp = 0) {
            variant BaseDisp {
                match {
                    ADD ptr:$base, imm(i32):$disp;
                };
                when {
                    isSimm32($disp);
                };
            };
            variant BaseOnly {
                match {
                    ptr:$base;
                };
            };
        };
    )";

    ParseContext ctx = createParseContextFromBuff("test", test);
    auto res = ctx.parse<DSL::Parser::InstructionSelectDef::AddrModeDeclRule,
                         DSL::Ast::InstructionSelectDef::AddrModeDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_modeName.m_node, "AddrModeRegImm");
    ASSERT_EQ(res->m_params.size(), 2);
    EXPECT_EQ(res->m_params[0].m_typeOrClass.m_node, "GPR64");
    EXPECT_EQ(res->m_params[0].m_name.m_node, "base");
    EXPECT_FALSE(res->m_params[0].m_defaultVal.has_value());

    EXPECT_EQ(res->m_params[1].m_typeOrClass.m_node, "simm32");
    EXPECT_EQ(res->m_params[1].m_name.m_node, "disp");
    ASSERT_TRUE(res->m_params[1].m_defaultVal.has_value());
    EXPECT_EQ(res->m_params[1].m_defaultVal->m_node, 0);

    ASSERT_EQ(res->m_variants.size(), 2);
    EXPECT_EQ(res->m_variants[0].m_variantName.m_node, "BaseDisp");
    EXPECT_EQ(res->m_variants[0].m_matchTree.m_opcode.m_node, "ADD");
    ASSERT_EQ(res->m_variants[0].m_whenClauses.size(), 1);
    EXPECT_EQ(res->m_variants[0].m_whenClauses[0].m_predicate.m_node, "isSimm32");

    EXPECT_EQ(res->m_variants[1].m_variantName.m_node, "BaseOnly");
}

/**
 * Verifies a basic selection pattern parses its name, cost, match tree, and
 * register-class-annotated select operands.
 */
TEST_F(InstructionSelectDefLangTest, TestSimpleSelectionPattern)
{
    std::string test = R"(
        pattern Select_ADD32rr [cost = 1] {
            match {
                ADD i32:$dst, i32:$src1, i32:$src2;
            };
            select {
                ADD32rr GPR32:$dst, GPR32:$src1, GPR32:$src2;
            };
        };
    )";

    ParseContext ctx = createParseContextFromBuff("test", test);
    auto res = ctx.parse<DSL::Parser::InstructionSelectDef::SelectionPatternRule,
                         DSL::Ast::InstructionSelectDef::SelectionPattern>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "Select_ADD32rr");
    EXPECT_EQ(res->m_cost, 1);
    EXPECT_EQ(res->m_matchTree.m_opcode.m_node, "ADD");
    ASSERT_EQ(res->m_matchTree.m_operands.size(), 3);
    EXPECT_EQ(res->m_matchTree.m_operands[0].m_name.m_node, "dst");

    ASSERT_EQ(res->m_selectClauses.size(), 1);
    EXPECT_EQ(res->m_selectClauses[0].m_targetOpcode.m_node, "ADD32rr");
    ASSERT_EQ(res->m_selectClauses[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_selectClauses[0].m_operands[0].m_name.m_node, "dst");
    ASSERT_TRUE(res->m_selectClauses[0].m_operands[0].m_regClass.has_value());
    EXPECT_EQ(res->m_selectClauses[0].m_operands[0].m_regClass->m_node, "GPR32");
}

/**
 * Verifies a memory-folded pattern parses a nested LOAD operand, an addressing
 * mode reference with arguments, when clauses, and a memory select operand.
 */
TEST_F(InstructionSelectDefLangTest, TestMemoryFoldedSelectionPattern)
{
    std::string test = R"(
        pattern Select_ADD32rm [cost = 2] {
            match {
                ADD i32:$dst, i32:$src1, (LOAD i32:$tmp, AddrModeRegImm($base, $disp));
            };
            when {
                hasOneUse($tmp);
                noInterveningStore($tmp);
            };
            select {
                ADD32rm GPR32:$dst, GPR32:$src1, [$base, $disp];
            };
        };
    )";

    ParseContext ctx = createParseContextFromBuff("test", test);
    auto res = ctx.parse<DSL::Parser::InstructionSelectDef::SelectionPatternRule,
                         DSL::Ast::InstructionSelectDef::SelectionPattern>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "Select_ADD32rm");
    EXPECT_EQ(res->m_cost, 2);
    EXPECT_EQ(res->m_matchTree.m_opcode.m_node, "ADD");
    ASSERT_EQ(res->m_matchTree.m_operands.size(), 3);

    // Third operand is nested LOAD
    const auto &nestedOp = res->m_matchTree.m_operands[2];
    EXPECT_EQ(nestedOp.m_kind, DSL::Ast::InstructionSelectDef::PatternOperand::Kind::NestedTree);
    ASSERT_NE(nestedOp.m_nestedTree, nullptr);
    EXPECT_EQ(nestedOp.m_nestedTree->m_opcode.m_node, "LOAD");
    ASSERT_EQ(nestedOp.m_nestedTree->m_operands.size(), 2);
    EXPECT_EQ(nestedOp.m_nestedTree->m_operands[0].m_name.m_node, "tmp");
    EXPECT_EQ(nestedOp.m_nestedTree->m_operands[1].m_kind,
              DSL::Ast::InstructionSelectDef::PatternOperand::Kind::AddrModeRef);
    EXPECT_EQ(nestedOp.m_nestedTree->m_operands[1].m_name.m_node, "AddrModeRegImm");
    ASSERT_EQ(nestedOp.m_nestedTree->m_operands[1].m_addrModeArgs.size(), 2);
    EXPECT_EQ(nestedOp.m_nestedTree->m_operands[1].m_addrModeArgs[0].m_node, "base");
    EXPECT_EQ(nestedOp.m_nestedTree->m_operands[1].m_addrModeArgs[1].m_node, "disp");

    // When clauses
    ASSERT_EQ(res->m_whenClauses.size(), 2);
    EXPECT_EQ(res->m_whenClauses[0].m_predicate.m_node, "hasOneUse");
    EXPECT_EQ(res->m_whenClauses[1].m_predicate.m_node, "noInterveningStore");

    // Select clauses
    ASSERT_EQ(res->m_selectClauses.size(), 1);
    EXPECT_EQ(res->m_selectClauses[0].m_targetOpcode.m_node, "ADD32rm");
    ASSERT_EQ(res->m_selectClauses[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_selectClauses[0].m_operands[2].m_kind,
              DSL::Ast::InstructionSelectDef::TargetEmitOperand::Kind::AddrModeMem);
    ASSERT_EQ(res->m_selectClauses[0].m_operands[2].m_memOperands.size(), 2);
    EXPECT_EQ(res->m_selectClauses[0].m_operands[2].m_memOperands[0].m_node, "base");
    EXPECT_EQ(res->m_selectClauses[0].m_operands[2].m_memOperands[1].m_node, "disp");
}

/**
 * Verifies a full instruction select file parses the target name and multiple
 * selection patterns.
 */
TEST_F(InstructionSelectDefLangTest, TestInstructionSelectFile)
{
    std::string test = R"(
        target AMD64;

        pattern Select_ADD32rr [cost = 1] {
            match {
                ADD i32:$dst, i32:$src1, i32:$src2;
            };
            select {
                ADD32rr GPR32:$dst, GPR32:$src1, GPR32:$src2;
            };
        };

        pattern Select_ADD32ri [cost = 1] {
            match {
                ADD i32:$dst, i32:$src1, imm(i32):$imm;
            };
            when {
                isSimm32($imm);
            };
            select {
                ADD32ri GPR32:$dst, GPR32:$src1, $imm;
            };
        };
    )";

    ParseContext ctx = createParseContextFromBuff("test", test);
    auto res = ctx.parse<DSL::Parser::InstructionSelectDef::InstructionSelectFile,
                         DSL::Ast::InstructionSelectDef::InstructionSelectFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(res->m_targetName.has_value());
    EXPECT_EQ(res->m_targetName->m_node, "AMD64");
    ASSERT_EQ(res->m_patterns.size(), 2);
    EXPECT_EQ(res->m_patterns[0].m_name.m_node, "Select_ADD32rr");
    EXPECT_EQ(res->m_patterns[1].m_name.m_node, "Select_ADD32ri");
}

/**
 * Verifies that when clauses accept string literals and identifiers such as hasExtension("avx").
 */
TEST_F(InstructionSelectDefLangTest, TestHasExtensionWhenClause)
{
    std::string test = R"(
        pattern Select_VADDPS [cost = 1] {
            match {
                FADD v4f32:$dst, v4f32:$src1, v4f32:$src2;
            };
            when {
                hasExtension("avx");
                hasFeature(sse4_1);
            };
            select {
                VADDPS VR128:$dst, VR128:$src1, VR128:$src2;
            };
        };
    )";

    ParseContext ctx = createParseContextFromBuff("test", test);
    auto res = ctx.parse<DSL::Parser::InstructionSelectDef::SelectionPatternRule,
                         DSL::Ast::InstructionSelectDef::SelectionPattern>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "Select_VADDPS");
    ASSERT_EQ(res->m_whenClauses.size(), 2u);

    EXPECT_EQ(res->m_whenClauses[0].m_predicate.m_node, "hasExtension");
    ASSERT_EQ(res->m_whenClauses[0].m_args.size(), 1u);
    EXPECT_EQ(res->m_whenClauses[0].m_args[0].m_node, "avx");

    EXPECT_EQ(res->m_whenClauses[1].m_predicate.m_node, "hasFeature");
    ASSERT_EQ(res->m_whenClauses[1].m_args.size(), 1u);
    EXPECT_EQ(res->m_whenClauses[1].m_args[0].m_node, "sse4_1");
}

