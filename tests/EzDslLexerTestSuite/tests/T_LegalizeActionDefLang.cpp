#include "EzDslLexerTestSuite.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

class LegalizeActionLangTest : public DslLexerTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. TypeConstraint Parsing
// ============================================================================

TEST_F(LegalizeActionLangTest, TestHomogeneousTypeConstraint)
{
    std::string test = "i32";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type.m_node, "i32");
    EXPECT_FALSE(res->m_operandIndex.has_value());
}

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

TEST_F(LegalizeActionLangTest, TestCustomActionClauses)
{
    std::string customTest = "CUSTOM() >> MyCustomAct";
    ParseContext customCtx = createParseContextFromBuff("customTest", customTest);

    auto customRes = customCtx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                                     DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(customRes.has_value());
    EXPECT_EQ(customRes->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom);
    ASSERT_TRUE(customRes->m_customRules.has_value());
    EXPECT_EQ(customRes->m_customRules.value()[0].m_node, "MyCustomAct");
}

// ============================================================================
// 3. LegalizeInstructionDecl Parsing
// ============================================================================

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

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeInstructionDecl,
                         DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_instName.m_node, "ADD");
    ASSERT_EQ(res->m_actionClauses.size(), 3);

    EXPECT_EQ(res->m_actionClauses[0].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Legal);
    ASSERT_EQ(res->m_actionClauses[0].m_types.size(), 2);
    EXPECT_EQ(res->m_actionClauses[0].m_types[0].m_type.m_node, "i32");
    EXPECT_EQ(res->m_actionClauses[0].m_types[1].m_type.m_node, "f32");

    EXPECT_EQ(res->m_actionClauses[1].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar);
    EXPECT_EQ(res->m_actionClauses[1].m_targetType->m_node, "i32");

    EXPECT_EQ(res->m_actionClauses[2].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar);
    EXPECT_EQ(res->m_actionClauses[2].m_targetType->m_node, "i32");
}

TEST_F(LegalizeActionLangTest, TestHeterogeneousInstructionDeclaration)
{
    std::string test = R"(
action SEXT {
    LEGAL(i32, i64) >> i64;
    WIDENS(i1:1, i8:1, i16:1) >> i32;
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeInstructionDecl,
                         DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_instName.m_node, "SEXT");
    ASSERT_EQ(res->m_actionClauses.size(), 2);

    ASSERT_EQ(res->m_actionClauses[1].m_types.size(), 3);
    EXPECT_EQ(res->m_actionClauses[1].m_types[0].m_type.m_node, "i1");
    ASSERT_TRUE(res->m_actionClauses[1].m_types[0].m_operandIndex.has_value());
    EXPECT_EQ(res->m_actionClauses[1].m_types[0].m_operandIndex->m_node, 1);
}

// ============================================================================
// 4. LegalizeActionFile Full Parsing
// ============================================================================

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

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeActionFile,
                         DSL::Ast::LegalizeActionDef::LegalizeActionFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_legalizeInstrDecls.size(), 3);

    EXPECT_EQ(res->m_legalizeInstrDecls[0].m_instName.m_node, "ADD");
    EXPECT_EQ(res->m_legalizeInstrDecls[0].m_actionClauses.size(), 3);

    EXPECT_EQ(res->m_legalizeInstrDecls[1].m_instName.m_node, "SDIV");
    EXPECT_EQ(res->m_legalizeInstrDecls[1].m_actionClauses.size(), 3);
    EXPECT_EQ(res->m_legalizeInstrDecls[1].m_actionClauses[1].m_kind,
              DSL::Ast::LegalizeActionDef::LegalizeActionKind::Libcall);
    EXPECT_EQ(res->m_legalizeInstrDecls[1].m_actionClauses[1].m_libcallSymbol->m_node, "__divdi3");

    EXPECT_EQ(res->m_legalizeInstrDecls[2].m_instName.m_node, "BITCAST");
    EXPECT_EQ(res->m_legalizeInstrDecls[2].m_actionClauses.size(), 2);
}

// ============================================================================
// 5. Negative & Error Parsing Tests
// ============================================================================

TEST_F(LegalizeActionLangTest, TestMissingSemicolonInActionClauseError)
{
    std::string test = R"(
action ADD {
    LEGAL(i32) >> i32
    NARROWS(i64) >> i32;
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeInstructionDecl,
                         DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeActionLangTest, TestMissingClosingBraceError)
{
    std::string test = R"(
action ADD {
    LEGAL(i32) >> i32;
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeInstructionDecl,
                         DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeActionLangTest, TestDanglingColonInTypeConstraintError)
{
    std::string test = "i32:";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeActionLangTest, TestMissingCommaSeparatorInTypesError)
{
    std::string test = "WIDENS(i8 i16) >> i32";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeActionLangTest, TestUnknownActionKindError)
{
    std::string test = "UNKNOWN_ACTION(i32) >> i64";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeActionLangTest, TestMissingActionKeywordError)
{
    std::string test = R"(
ADD {
    LEGAL(i32) >> i32;
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeInstructionDecl,
                         DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl>();
    EXPECT_FALSE(res.has_value());
}

// ============================================================================
// 6. Extended EzDSL Legalizer (.lad) Features
// ============================================================================

TEST_F(LegalizeActionLangTest, TestClampScalarClause)
{
    std::string test = "CLAMP_SCALAR(i32, i64)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::ClampScalarClause,
                         DSL::Ast::LegalizeActionDef::ClampScalarClause>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_minType.m_node, "i32");
    EXPECT_EQ(res->m_maxType.m_node, "i64");
}

TEST_F(LegalizeActionLangTest, TestLowerActionClause)
{
    std::string test = "LOWER >> AMD64CallLowering";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Lower);
    ASSERT_TRUE(res->m_lowerHandler.has_value());
    EXPECT_EQ(res->m_lowerHandler->m_node, "AMD64CallLowering");
}

TEST_F(LegalizeActionLangTest, TestTypeSetDeclaration)
{
    std::string test = "type_set GPR_SCALARS = (i8, i16, i32, i64);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeSetDecl,
                         DSL::Ast::LegalizeActionDef::TypeSetDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "GPR_SCALARS");
    ASSERT_EQ(res->m_types.size(), 4);
    EXPECT_EQ(res->m_types[0].m_node, "i8");
    EXPECT_EQ(res->m_types[1].m_node, "i16");
    EXPECT_EQ(res->m_types[2].m_node, "i32");
    EXPECT_EQ(res->m_types[3].m_node, "i64");
}

TEST_F(LegalizeActionLangTest, TestInstructionGroupDeclaration)
{
    std::string test = R"(
group IntegerALU = (SUB, AND, OR, XOR) {
    CLAMP_SCALAR(i32, i64);
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionGroupDecl,
                         DSL::Ast::LegalizeActionDef::InstructionGroupDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_groupName.m_node, "IntegerALU");
    ASSERT_EQ(res->m_instructions.size(), 4);
    EXPECT_EQ(res->m_instructions[0].m_node, "SUB");
    EXPECT_EQ(res->m_instructions[1].m_node, "AND");
    EXPECT_EQ(res->m_instructions[2].m_node, "OR");
    EXPECT_EQ(res->m_instructions[3].m_node, "XOR");
    ASSERT_TRUE(res->m_clampClause.has_value());
    EXPECT_EQ(res->m_clampClause->m_minType.m_node, "i32");
    EXPECT_EQ(res->m_clampClause->m_maxType.m_node, "i64");
}

TEST_F(LegalizeActionLangTest, TestFullTargetWithTargetHeaderAndGroups)
{
    std::string test = R"dsl(
target AMD64;

type_set GPR_SCALARS = (i8, i16, i32, i64);

action ADD {
    CLAMP_SCALAR(i32, i64);
};

group IntegerALU = (SUB, AND, OR, XOR) {
    CLAMP_SCALAR(i32, i64);
};

action CALL {
    LOWER >> AMD64CallLowering;
};
)dsl";

    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeActionFile,
                         DSL::Ast::LegalizeActionDef::LegalizeActionFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(res->m_targetName.has_value());
    EXPECT_EQ(res->m_targetName->m_node, "AMD64");

    ASSERT_EQ(res->m_typeSets.size(), 1);
    EXPECT_EQ(res->m_typeSets[0].m_name.m_node, "GPR_SCALARS");

    ASSERT_EQ(res->m_groups.size(), 1);
    EXPECT_EQ(res->m_groups[0].m_groupName.m_node, "IntegerALU");

    ASSERT_EQ(res->m_legalizeInstrDecls.size(), 2);
    EXPECT_EQ(res->m_legalizeInstrDecls[0].m_instName.m_node, "ADD");
    ASSERT_TRUE(res->m_legalizeInstrDecls[0].m_clampClause.has_value());
    EXPECT_EQ(res->m_legalizeInstrDecls[1].m_instName.m_node, "CALL");
    ASSERT_EQ(res->m_legalizeInstrDecls[1].m_actionClauses.size(), 1);
    EXPECT_EQ(res->m_legalizeInstrDecls[1].m_actionClauses[0].m_kind,
              DSL::Ast::LegalizeActionDef::LegalizeActionKind::Lower);
    EXPECT_EQ(res->m_legalizeInstrDecls[1].m_actionClauses[0].m_lowerHandler->m_node,
              "AMD64CallLowering");
}