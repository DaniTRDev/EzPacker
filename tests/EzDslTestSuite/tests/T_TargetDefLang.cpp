#include "EzDslTestSuite.h"
#include "Ast/TargetDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "SourceManager/SourceManager.h"

/**
 * Test fixture for Target Definition Language (.tdf) parser, register bank declarations, and include directives.
 */
class TargetDefLangTest : public DslTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. Hardware & Virtual Register Declarations
// ============================================================================

/**
 * Verifies parsing root hardware register declarations without parent aliases (e.g. rax(, 64, 0)).
 */
TEST_F(TargetDefLangTest, TestRootRegisterWithoutParent)
{
    std::string test = "rax(, 64, 0)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegister, DSL::Ast::TargetDef::TargetRegister>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "rax");
    EXPECT_TRUE(res->m_parentName.m_node.empty());
    EXPECT_EQ(res->m_size.m_node, 64);
    EXPECT_EQ(res->m_offset.m_node, 0);
}

/**
 * Verifies parsing sub-register aliases referencing a parent register at bit offset 0 (e.g. eax(rax, 32, 0)).
 */
TEST_F(TargetDefLangTest, TestAliasedSubRegister)
{
    std::string test = "eax(rax, 32, 0)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegister, DSL::Ast::TargetDef::TargetRegister>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "eax");
    EXPECT_EQ(res->m_parentName.m_node, "rax");
    EXPECT_EQ(res->m_size.m_node, 32);
    EXPECT_EQ(res->m_offset.m_node, 0);
}

/**
 * Verifies parsing high-byte sub-register aliases with non-zero bit offsets (e.g. ah(ax, 8, 8)).
 */
TEST_F(TargetDefLangTest, TestHighByteSubRegisterWithOffset)
{
    std::string test = "ah(ax, 8, 8)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegister, DSL::Ast::TargetDef::TargetRegister>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "ah");
    EXPECT_EQ(res->m_parentName.m_node, "ax");
    EXPECT_EQ(res->m_size.m_node, 8);
    EXPECT_EQ(res->m_offset.m_node, 8);
}

// ============================================================================
// 2. Register Classes & Register Banks
// ============================================================================

/**
 * Verifies parsing empty register class declarations (CLASS(name);).
 */
TEST_F(TargetDefLangTest, TestEmptyRegisterClass)
{
    std::string test = "CLASS(gpr64);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegisterClass, DSL::Ast::TargetDef::TargetRegisterClass>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "gpr64");
    EXPECT_TRUE(res->m_registers.empty());
}

/**
 * Verifies parsing register classes containing multiple register declarations.
 */
TEST_F(TargetDefLangTest, TestRegisterClassWithRegisters)
{
    std::string test = R"(
CLASS(gpr64,
    rax(, 64, 0),
    rcx(, 64, 0),
    rdx(, 64, 0)
);
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegisterClass, DSL::Ast::TargetDef::TargetRegisterClass>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "gpr64");
    ASSERT_EQ(res->m_registers.size(), 3);

    EXPECT_EQ(res->m_registers[0].m_name.m_node, "rax");
    EXPECT_TRUE(res->m_registers[0].m_parentName.m_node.empty());
    EXPECT_EQ(res->m_registers[0].m_size.m_node, 64);
    EXPECT_EQ(res->m_registers[0].m_offset.m_node, 0);

    EXPECT_EQ(res->m_registers[1].m_name.m_node, "rcx");
    EXPECT_EQ(res->m_registers[2].m_name.m_node, "rdx");
}

/**
 * Verifies parsing register bank definitions containing multiple register classes.
 */
TEST_F(TargetDefLangTest, TestRegisterBankMultipleClasses)
{
    std::string test = R"(
bank GPR {
    CLASS(gpr64,
        rax(, 64, 0),
        rbx(, 64, 0)
    );
    CLASS(gpr32,
        eax(rax, 32, 0),
        ebx(rbx, 32, 0)
    );
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegisterBank, DSL::Ast::TargetDef::TargetRegisterBank>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "GPR");
    ASSERT_EQ(res->m_classes.size(), 2);

    EXPECT_EQ(res->m_classes[0].m_name.m_node, "gpr64");
    ASSERT_EQ(res->m_classes[0].m_registers.size(), 2);
    EXPECT_EQ(res->m_classes[0].m_registers[0].m_name.m_node, "rax");

    EXPECT_EQ(res->m_classes[1].m_name.m_node, "gpr32");
    ASSERT_EQ(res->m_classes[1].m_registers.size(), 2);
    EXPECT_EQ(res->m_classes[1].m_registers[0].m_name.m_node, "eax");
    EXPECT_EQ(res->m_classes[1].m_registers[0].m_parentName.m_node, "rax");
}

// ============================================================================
// 3. File Inclusions & Full Target Declarations
// ============================================================================

/**
 * Verifies parsing multiple include directives inside a target block (including .idf, .lad, .lrd, .isf files).
 */
TEST_F(TargetDefLangTest, TestMultipleInclusions)
{
    std::string test = R"(
target RISCV64 {
    include idf "instructions.idf";
    include lad "legalizeAction.lad";
    include lrd "legalizeRule.lrd";
    include isf "instructionSel.isf";
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "RISCV64");
    ASSERT_EQ(res->m_inclusions.size(), 4);

    EXPECT_EQ(res->m_inclusions[0].m_inclusionType, DSL::Ast::TargetDef::TargetIncludeFileType::InstructionDef);
    EXPECT_EQ(res->m_inclusions[0].m_path.m_node, "instructions.idf");

    EXPECT_EQ(res->m_inclusions[1].m_inclusionType, DSL::Ast::TargetDef::TargetIncludeFileType::LegalizeActionDef);
    EXPECT_EQ(res->m_inclusions[1].m_path.m_node, "legalizeAction.lad");

    EXPECT_EQ(res->m_inclusions[2].m_inclusionType, DSL::Ast::TargetDef::TargetIncludeFileType::LegalizeRuleDef);
    EXPECT_EQ(res->m_inclusions[2].m_path.m_node, "legalizeRule.lrd");

    EXPECT_EQ(res->m_inclusions[3].m_inclusionType, DSL::Ast::TargetDef::TargetIncludeFileType::InstructionSelDef);
    EXPECT_EQ(res->m_inclusions[3].m_path.m_node, "instructionSel.isf");

    EXPECT_TRUE(res->m_regBanks.empty());
}

/**
 * Verifies parsing a complete target definition with interleaved register bank definitions and include directives.
 */
TEST_F(TargetDefLangTest, TestCompleteTargetDefinitionInterleaved)
{
    std::string test = R"(
target x86_64 {
    include idf "x86_insts.idf";

    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0),
            rcx(, 64, 0)
        );
        CLASS(gpr32,
            eax(rax, 32, 0),
            ecx(rcx, 32, 0)
        );
    };

    include isf "x86_patterns.isf";

    bank FPR {
        CLASS(fpr64,
            xmm0(, 64, 0),
            xmm1(, 64, 0)
        );
    };
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "x86_64");

    // Inclusions validation
    ASSERT_EQ(res->m_inclusions.size(), 2);
    EXPECT_EQ(res->m_inclusions[0].m_inclusionType, DSL::Ast::TargetDef::TargetIncludeFileType::InstructionDef);
    EXPECT_EQ(res->m_inclusions[0].m_path.m_node, "x86_insts.idf");
    EXPECT_EQ(res->m_inclusions[1].m_inclusionType, DSL::Ast::TargetDef::TargetIncludeFileType::InstructionSelDef);
    EXPECT_EQ(res->m_inclusions[1].m_path.m_node, "x86_patterns.isf");

    // Register banks validation
    ASSERT_EQ(res->m_regBanks.size(), 2);
    EXPECT_EQ(res->m_regBanks[0].m_name.m_node, "GPR");
    ASSERT_EQ(res->m_regBanks[0].m_classes.size(), 2);
    EXPECT_EQ(res->m_regBanks[0].m_classes[0].m_name.m_node, "gpr64");
    EXPECT_EQ(res->m_regBanks[0].m_classes[1].m_name.m_node, "gpr32");

    EXPECT_EQ(res->m_regBanks[1].m_name.m_node, "FPR");
    ASSERT_EQ(res->m_regBanks[1].m_classes.size(), 1);
    EXPECT_EQ(res->m_regBanks[1].m_classes[0].m_name.m_node, "fpr64");
}

// ============================================================================
// 4. Negative & Error Parsing Tests
// ============================================================================

/**
 * Verifies syntax error rejection when a target definition block is empty.
 */
TEST_F(TargetDefLangTest, TestEmptyTargetBodyFails)
{
    std::string test = R"(
target MyTarget {
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a register declaration is missing its offset argument.
 */
TEST_F(TargetDefLangTest, TestMalformedRegisterMissingOffset)
{
    std::string test = "rax(, 64)"; // Missing comma and offset parameter
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegister, DSL::Ast::TargetDef::TargetRegister>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a register class declaration is missing its terminating semicolon.
 */
TEST_F(TargetDefLangTest, TestMalformedClassMissingSemicolon)
{
    std::string test = R"(
CLASS(gpr64,
    rax(, 64, 0)
)
)"; // Missing closing semicolon
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegisterClass, DSL::Ast::TargetDef::TargetRegisterClass>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when an unrecognized statement is encountered in a target block.
 */
TEST_F(TargetDefLangTest, TestUnknownBodyItemInTarget)
{
    std::string test = R"(
target MyTarget {
    foo bar "file.idf";
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a target definition block is unclosed.
 */
TEST_F(TargetDefLangTest, TestUnterminatedTargetDef)
{
    std::string test = R"(
target MyTarget {
    include idef "file.idf";
)"; // Missing closing brace '}'
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    EXPECT_FALSE(res.has_value());
}