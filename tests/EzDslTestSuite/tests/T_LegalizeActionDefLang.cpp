#include "EzDslTestSuite.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

/**
 * Test fixture for Legalize Action Definition Language (.lad) parser, grammar rules, and AST construction.
 */
class LegalizeActionLangTest : public DslTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. TypeConstraint Parsing
// ============================================================================

/**
 * Verifies parsing homogeneous scalar type constraints without operand indices (e.g. i32).
 */
TEST_F(LegalizeActionLangTest, TestHomogeneousTypeConstraint)
{
    std::string test = "i32";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type.m_node, "i32");
    EXPECT_FALSE(res->m_operandIndex.has_value());
}

/**
 * Verifies parsing heterogeneous type constraints with explicit operand index specifications (e.g. i8:1).
 */
TEST_F(LegalizeActionLangTest, TestHeterogeneousTypeConstraintWithIndex)
{
    std::string test = "i8:1";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type.m_node, "i8");
    ASSERT_TRUE(res->m_operandIndex.has_value());
    EXPECT_EQ(res->m_operandIndex->m_node, 1);
}

/**
 * Verifies parsing vector and complex type constraints in legalization clauses.
 */
TEST_F(LegalizeActionLangTest, TestPointerAndVectorTypeConstraints)
{
    std::string test = "v4f32";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type.m_node, "v4f32");
    EXPECT_FALSE(res->m_operandIndex.has_value());
}

// ============================================================================
// 2. LegalizationClause Parsing
// ============================================================================

/**
 * Verifies parsing scalar widening clauses (WIDENS(types...) >> targetType).
 */
TEST_F(LegalizeActionLangTest, TestWidenActionClause)
{
    std::string test = "WIDENS(i1, i8, i16) >> i32";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar);
    ASSERT_EQ(res->m_types.size(), 3);
    EXPECT_EQ(res->m_types[0].m_type.m_node, "i1");
    EXPECT_EQ(res->m_types[1].m_type.m_node, "i8");
    EXPECT_EQ(res->m_types[2].m_type.m_node, "i16");

    ASSERT_TRUE(res->m_targetType.has_value());
    EXPECT_EQ(res->m_targetType->m_node, "i32");
    EXPECT_FALSE(res->m_libcallSymbol.has_value());
}

/**
 * Verifies parsing scalar narrowing clauses (NARROWS(types...) >> targetType).
 */
TEST_F(LegalizeActionLangTest, TestNarrowActionClause)
{
    std::string test = "NARROWS(i64) >> i32";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar);
    ASSERT_EQ(res->m_types.size(), 1);
    EXPECT_EQ(res->m_types[0].m_type.m_node, "i64");

    ASSERT_TRUE(res->m_targetType.has_value());
    EXPECT_EQ(res->m_targetType->m_node, "i32");
    EXPECT_FALSE(res->m_libcallSymbol.has_value());
}

/**
 * Verifies parsing runtime library call lowering clauses (LIBCALL(types...) >> "symbol").
 */
TEST_F(LegalizeActionLangTest, TestLibcallActionClause)
{
    std::string test = "LIBCALL(i64) >> \"__divdi3\"";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Libcall);
    ASSERT_EQ(res->m_types.size(), 1);
    EXPECT_EQ(res->m_types[0].m_type.m_node, "i64");

    ASSERT_TRUE(res->m_libcallSymbol.has_value());
    EXPECT_EQ(res->m_libcallSymbol->m_node, "__divdi3");
    EXPECT_FALSE(res->m_targetType.has_value());
}

/**
 * Verifies parsing type bitcast transformation clauses (BITCAST(types...) >> targetType).
 */
TEST_F(LegalizeActionLangTest, TestBitcastActionClause)
{
    std::string test = "BITCAST(f32) >> i32";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Bitcast);
    ASSERT_EQ(res->m_types.size(), 1);
    EXPECT_EQ(res->m_types[0].m_type.m_node, "f32");

    ASSERT_TRUE(res->m_targetType.has_value());
    EXPECT_EQ(res->m_targetType->m_node, "i32");
    EXPECT_FALSE(res->m_libcallSymbol.has_value());
}

/**
 * Verifies parsing custom legalization hooks (CUSTOM) and unsupported operation markers (UNSUPPORTED).
 */
TEST_F(LegalizeActionLangTest, TestCustomAndUnsupportedActionClauses)
{
    std::string customTest = "CUSTOM(i128) >> i64";
    ParseContext customCtx = createParseContextFromBuff("customTest", customTest);

    auto customRes = customCtx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                                     DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(customRes.has_value());
    EXPECT_EQ(customRes->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom);
    EXPECT_EQ(customRes->m_targetType->m_node, "i64");

    std::string unsuppTest = "UNSUPPORTED(f128) >> f128";
    ParseContext unsuppCtx = createParseContextFromBuff("unsuppTest", unsuppTest);

    auto unsuppRes = unsuppCtx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                                     DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(unsuppRes.has_value());
    EXPECT_EQ(unsuppRes->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Unsupported);
}

// ============================================================================
// 3. InstructionLegalizeDecl Parsing (Block Syntax with '{' ... '}')
// ============================================================================

/**
 * Verifies parsing an instruction legalization block containing multiple action clauses (LEGAL, WIDENS, NARROWS).
 */
TEST_F(LegalizeActionLangTest, TestInstructionLegalizeDeclaration)
{
    std::string test = R"(
action ADD {
    LEGAL(i32, f32) >> i32;
    WIDENS(i1, i8, i16) >> i32;
    NARROWS(i64) >> i32;
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_instName.m_node, "ADD");
    ASSERT_EQ(res->m_actions.size(), 3);

    EXPECT_EQ(res->m_actions[0].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Legal);
    ASSERT_EQ(res->m_actions[0].m_types.size(), 2);
    EXPECT_EQ(res->m_actions[0].m_types[0].m_type.m_node, "i32");
    EXPECT_EQ(res->m_actions[0].m_types[1].m_type.m_node, "f32");

    EXPECT_EQ(res->m_actions[1].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar);
    EXPECT_EQ(res->m_actions[1].m_targetType->m_node, "i32");

    EXPECT_EQ(res->m_actions[2].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar);
    EXPECT_EQ(res->m_actions[2].m_targetType->m_node, "i32");
}

/**
 * Verifies parsing instruction legalization declarations with heterogeneous operand indexing (e.g. SEXT).
 */
TEST_F(LegalizeActionLangTest, TestHeterogeneousInstructionDeclaration)
{
    std::string test = R"(
action SEXT {
    LEGAL(i32, i64) >> i64;
    WIDENS(i1:1, i8:1, i16:1) >> i32;
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_instName.m_node, "SEXT");
    ASSERT_EQ(res->m_actions.size(), 2);

    ASSERT_EQ(res->m_actions[1].m_types.size(), 3);
    EXPECT_EQ(res->m_actions[1].m_types[0].m_type.m_node, "i1");
    ASSERT_TRUE(res->m_actions[1].m_types[0].m_operandIndex.has_value());
    EXPECT_EQ(res->m_actions[1].m_types[0].m_operandIndex->m_node, 1);
}

// ============================================================================
// 4. TargetLegalizeDef Full Translation Unit Parsing
// ============================================================================

/**
 * Verifies parsing an entire target legalization definition file (.lad) with multiple instruction blocks.
 */
TEST_F(LegalizeActionLangTest, TestFullTargetLegalizeDefinitionFile)
{
    std::string test = R"dsl(
action ADD {
    LEGAL(i32, f32) >> i32;
    WIDENS(i1, i8, i16) >> i32;
    NARROWS(i64) >> i32;
};

action SDIV {
    LEGAL(i32) >> i32;
    LIBCALL(i64) >> "__divdi3";
    UNSUPPORTED(f32) >> f32;
};

action BITCAST {
    BITCAST(f32) >> i32;
    BITCAST(i32) >> f32;
};
)dsl";

    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TargetLegalizeDef,
                         DSL::Ast::LegalizeActionDef::TargetLegalizeDef>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_instructionActions.size(), 3);

    EXPECT_EQ(res->m_instructionActions[0].m_instName.m_node, "ADD");
    EXPECT_EQ(res->m_instructionActions[0].m_actions.size(), 3);

    EXPECT_EQ(res->m_instructionActions[1].m_instName.m_node, "SDIV");
    EXPECT_EQ(res->m_instructionActions[1].m_actions.size(), 3);
    EXPECT_EQ(res->m_instructionActions[1].m_actions[1].m_kind,
              DSL::Ast::LegalizeActionDef::LegalizeActionKind::Libcall);
    EXPECT_EQ(res->m_instructionActions[1].m_actions[1].m_libcallSymbol->m_node, "__divdi3");

    EXPECT_EQ(res->m_instructionActions[2].m_instName.m_node, "BITCAST");
    EXPECT_EQ(res->m_instructionActions[2].m_actions.size(), 2);
}

// ============================================================================
// 5. Negative & Error Parsing Tests
// ============================================================================

/**
 * Verifies syntax error rejection when a semicolon is missing inside an action clause.
 */
TEST_F(LegalizeActionLangTest, TestMissingSemicolonInActionClauseError)
{
    std::string test = R"(
action ADD {
    LEGAL(i32) >> i32
    NARROWS(i64) >> i32;
};
)"; // Missing ';' after first clause
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when an action block closing brace is missing.
 */
TEST_F(LegalizeActionLangTest, TestMissingClosingBraceError)
{
    std::string test = R"(
action ADD {
    LEGAL(i32) >> i32;
)"; // Unclosed '{'
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a dangling colon is present without an operand index.
 */
TEST_F(LegalizeActionLangTest, TestDanglingColonInTypeConstraintError)
{
    std::string test = "i32:"; // Colon present but missing integer slot index
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when comma separators are missing in type parameter lists.
 */
TEST_F(LegalizeActionLangTest, TestMissingCommaSeparatorInTypesError)
{
    std::string test = "WIDENS(i8 i16) >> i32"; // Missing ',' separator
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when an unknown action verb is used.
 */
TEST_F(LegalizeActionLangTest, TestUnknownActionKindError)
{
    std::string test = "UNKNOWN_ACTION(i32) >> i64";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when the leading 'action' keyword is omitted.
 */
TEST_F(LegalizeActionLangTest, TestMissingActionKeywordError)
{
    std::string test = R"(
ADD {
    LEGAL(i32) >> i32;
};
)"; // Missing leading 'action' keyword
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    EXPECT_FALSE(res.has_value());
}