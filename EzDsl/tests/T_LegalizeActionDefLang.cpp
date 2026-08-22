#include "EzDslCommon.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"
#include <gtest/gtest.h>

class LegalizeMatrixLangTest : public ::testing::Test
{
  public:
    DiagnosticCollector *getDiagCollector() { return m_diagCollector.get(); }

    size_t addSource(const std::string &source, const std::string &content)
    {
        return m_sourceManager->addSourceContent(source, content);
    }

    SourceManager *getSourceManager() { return m_sourceManager.get(); }

    void SetUp() override
    {
        m_sourceManager = std::make_shared<SourceManager>("", &m_resource);
        m_diagLogger = std::make_shared<DiagnosticLogger>(m_sourceManager.get());
        m_diagCollector = std::make_shared<DiagnosticCollector>();

        m_diagCollector->addListener(m_diagLogger.get());
    }

    void TearDown() override {}

  private:
    std::pmr::monotonic_buffer_resource m_resource;
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
    std::shared_ptr<DiagnosticLogger> m_diagLogger;
    std::shared_ptr<SourceManager> m_sourceManager;
};

// ============================================================================
// 1. TypeConstraint Parsing
// ============================================================================

TEST_F(LegalizeMatrixLangTest, TestHomogeneousTypeConstraint)
{
    std::string test = "i32";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type.m_node, "i32");
    EXPECT_FALSE(res->m_typeIndex.has_value());
}

TEST_F(LegalizeMatrixLangTest, TestHeterogeneousTypeConstraintWithIndex)
{
    std::string test = "i8:1";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type.m_node, "i8");
    ASSERT_TRUE(res->m_typeIndex.has_value());
    EXPECT_EQ(res->m_typeIndex->m_node, 1);
}

TEST_F(LegalizeMatrixLangTest, TestPointerAndVectorTypeConstraints)
{
    std::string test = "v4f32";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_type.m_node, "v4f32");
    EXPECT_FALSE(res->m_typeIndex.has_value());
}

// ============================================================================
// 2. LegalizationClause Parsing
// ============================================================================

TEST_F(LegalizeMatrixLangTest, TestWidenActionClause)
{
    std::string test = "WIDENS(i1, i8, i16) >> i32";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

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

TEST_F(LegalizeMatrixLangTest, TestNarrowActionClause)
{
    std::string test = "NARROWS(i64) >> i32";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

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

TEST_F(LegalizeMatrixLangTest, TestLibcallActionClause)
{
    std::string test = "LIBCALL(i64) >> \"__divdi3\"";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

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

TEST_F(LegalizeMatrixLangTest, TestBitcastActionClause)
{
    std::string test = "BITCAST(f32) >> i32";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

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

TEST_F(LegalizeMatrixLangTest, TestCustomAndUnsupportedActionClauses)
{
    std::string customTest = "CUSTOM(i128) >> i64";
    size_t customSourceId = addSource("customTest", customTest);
    ParseContext customCtx(getDiagCollector(), getSourceManager(), customSourceId);

    auto customRes = customCtx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                                     DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(customRes.has_value());
    EXPECT_EQ(customRes->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom);
    EXPECT_EQ(customRes->m_targetType->m_node, "i64");

    std::string unsuppTest = "UNSUPPORTED(f128) >> f128";
    size_t unsuppSourceId = addSource("unsuppTest", unsuppTest);
    ParseContext unsuppCtx(getDiagCollector(), getSourceManager(), unsuppSourceId);

    auto unsuppRes = unsuppCtx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                                     DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    ASSERT_TRUE(unsuppRes.has_value());
    EXPECT_EQ(unsuppRes->m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Unsupported);
}

// ============================================================================
// 3. InstructionLegalizeDecl Parsing (Block Syntax with '{' ... '}')
// ============================================================================

TEST_F(LegalizeMatrixLangTest, TestInstructionLegalizeDeclaration)
{
    std::string test = R"(
action ADD {
    LEGAL(i32, f32) >> i32;
    WIDENS(i1, i8, i16) >> i32;
    NARROWS(i64) >> i32;
};
)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_instName.m_node, "ADD");
    ASSERT_EQ(res->m_actions.size(), 3);

    EXPECT_EQ(res->m_actions[0].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Legal);
    EXPECT_EQ(res->m_actions[0].m_types.size(), 2);
    EXPECT_EQ(res->m_actions[0].m_types[0].m_type.m_node, "i32");
    EXPECT_EQ(res->m_actions[0].m_types[1].m_type.m_node, "f32");

    EXPECT_EQ(res->m_actions[1].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar);
    EXPECT_EQ(res->m_actions[1].m_targetType->m_node, "i32");

    EXPECT_EQ(res->m_actions[2].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar);
    EXPECT_EQ(res->m_actions[2].m_targetType->m_node, "i32");
}

TEST_F(LegalizeMatrixLangTest, TestHeterogeneousInstructionDeclaration)
{
    std::string test = R"(
action SEXT {
    LEGAL(i32, i64) >> i64;
    WIDENS(i1:1, i8:1, i16:1) >> i32;
};
)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_instName.m_node, "SEXT");
    ASSERT_EQ(res->m_actions.size(), 2);

    ASSERT_EQ(res->m_actions[1].m_types.size(), 3);
    EXPECT_EQ(res->m_actions[1].m_types[0].m_type.m_node, "i1");
    ASSERT_TRUE(res->m_actions[1].m_types[0].m_typeIndex.has_value());
    EXPECT_EQ(res->m_actions[1].m_types[0].m_typeIndex->m_node, 1);
}

// ============================================================================
// 4. TargetLegalizeDef Full Translation Unit Parsing
// ============================================================================

TEST_F(LegalizeMatrixLangTest, TestFullTargetLegalizeDefinitionFile)
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

    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

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

TEST_F(LegalizeMatrixLangTest, TestMissingSemicolonInActionClauseError)
{
    std::string test = R"(
action ADD {
    LEGAL(i32) >> i32
    NARROWS(i64) >> i32;
};
)"; // Missing ';' after first clause
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeMatrixLangTest, TestMissingClosingBraceError)
{
    std::string test = R"(
action ADD {
    LEGAL(i32) >> i32;
)"; // Unclosed '{'
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeMatrixLangTest, TestDanglingColonInTypeConstraintError)
{
    std::string test = "i32:"; // Colon present but missing integer slot index
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::TypeConstraint, DSL::Ast::LegalizeActionDef::TypeConstraint>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeMatrixLangTest, TestMissingCommaSeparatorInTypesError)
{
    std::string test = "WIDENS(i8 i16) >> i32"; // Missing ',' separator
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeMatrixLangTest, TestUnknownActionKindError)
{
    std::string test = "UNKNOWN_ACTION(i32) >> i64";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::LegalizationClause,
                         DSL::Ast::LegalizeActionDef::LegalizeActionClause>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(LegalizeMatrixLangTest, TestMissingActionKeywordError)
{
    std::string test = R"(
ADD {
    LEGAL(i32) >> i32;
};
)"; // Missing leading 'action' keyword
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::LegalizeActionDef::InstructionLegalizeDecl,
                         DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl>();
    EXPECT_FALSE(res.has_value());
}