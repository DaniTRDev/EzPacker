#include "EzDslTestSuite.h"
#include "Ast/TypeDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/ParseContext.h"
#include "Parser/TypeDefLang.h"
#include "SourceManager/SourceManager.h"

/**
 * Test fixture for Type Definition Language (.tyf) parser, scalar type descriptors, and AST generation.
 */
class TypeDefLangTest : public DslTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. Single Type Descriptor Declarations
// ============================================================================

/**
 * Verifies parsing an integer type descriptor with explicit bit width (e.g. integer i32(32)).
 */
TEST_F(TypeDefLangTest, TestIntegerTypeDescriptor)
{
    std::string test = "integer i32(32, 64);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDescriptor, DSL::Ast::TypeDef::TypeDescriptor>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::TypeDef::TypeKind::Integer);
    EXPECT_EQ(res->m_name.m_node, "i32");
    ASSERT_TRUE(res->m_bitSize.has_value());
    EXPECT_EQ(res->m_bitSize->m_node, 32);
    ASSERT_TRUE(res->m_alignment.has_value());
    EXPECT_EQ(res->m_alignment->m_node, 64);
}

/**
 * Verifies parsing a floating-point type descriptor with explicit bit width (e.g. float f64(64)).
 */
TEST_F(TypeDefLangTest, TestFloatTypeDescriptor)
{
    std::string test = "float f64(64);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDescriptor, DSL::Ast::TypeDef::TypeDescriptor>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::TypeDef::TypeKind::FloatingPoint);
    EXPECT_EQ(res->m_name.m_node, "f64");
    ASSERT_TRUE(res->m_bitSize.has_value());
    EXPECT_EQ(res->m_bitSize->m_node, 64);
    ASSERT_FALSE(res->m_alignment.has_value());
}

/**
 * Verifies parsing arbitrary single-bit and custom bit-width integer types (e.g. integer i1(1)).
 */
TEST_F(TypeDefLangTest, TestCustomBitWidths)
{
    std::string test = "integer i1(1)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDescriptor, DSL::Ast::TypeDef::TypeDescriptor>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::TypeDef::TypeKind::Integer);
    EXPECT_EQ(res->m_name.m_node, "i1");
    ASSERT_TRUE(res->m_bitSize.has_value());
    EXPECT_EQ(res->m_bitSize->m_node, 1);
}

/**
 * Verifies parsing void type descriptor without bit size parameter.
 */
TEST_F(TypeDefLangTest, TestVoidTypeDescriptorWithoutBitSize)
{
    std::string test = "void void()";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDescriptor, DSL::Ast::TypeDef::TypeDescriptor>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::TypeDef::TypeKind::Void);
    EXPECT_EQ(res->m_name.m_node, "void");
    EXPECT_FALSE(res->m_bitSize.has_value());
}

/**
 * Verifies parsing void type descriptor with explicit zero bit size parameter (void void(0)).
 */
TEST_F(TypeDefLangTest, TestVoidTypeDescriptorWithBitSize)
{
    std::string test = "void void(0);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDescriptor, DSL::Ast::TypeDef::TypeDescriptor>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::TypeDef::TypeKind::Void);
    EXPECT_EQ(res->m_name.m_node, "void");
    ASSERT_TRUE(res->m_bitSize.has_value());
    EXPECT_EQ(res->m_bitSize->m_node, 0);
}

/**
 * Verifies parsing compiler binding token type descriptors.
 */
TEST_F(TypeDefLangTest, TestBindingTokenTypeDescriptor)
{
    std::string test = "bindingToken __bindToken();";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDescriptor, DSL::Ast::TypeDef::TypeDescriptor>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_kind, DSL::Ast::TypeDef::TypeKind::BindingToken);
    EXPECT_EQ(res->m_name.m_node, "__bindToken");
    EXPECT_FALSE(res->m_bitSize.has_value());
}

// ============================================================================
// 2. Type Definition File / Multi-Type Declarations
// ============================================================================

/**
 * Verifies parsing a single type declaration statement within a .tyf file.
 */
TEST_F(TypeDefLangTest, TestSingleTypeInFile)
{
    std::string test = "integer i8(8);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_types.size(), 1);

    EXPECT_EQ(res->m_types[0].m_kind, DSL::Ast::TypeDef::TypeKind::Integer);
    EXPECT_EQ(res->m_types[0].m_name.m_node, "i8");
    ASSERT_TRUE(res->m_types[0].m_bitSize.has_value());
    EXPECT_EQ(res->m_types[0].m_bitSize->m_node, 8);
}

/**
 * Verifies parsing an entire .tyf file containing multiple integer, float, void, and token declarations.
 */
TEST_F(TypeDefLangTest, TestMultipleTypesInFile)
{
    std::string test = R"(
        integer i8(8);
        integer i16(16);
        integer i32(32);
        integer i64(64);
        float f32(32);
        float f64(64);
        void void();
        bindingToken __bindToken();
    )";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_types.size(), 8);

    EXPECT_EQ(res->m_types[0].m_kind, DSL::Ast::TypeDef::TypeKind::Integer);
    EXPECT_EQ(res->m_types[0].m_name.m_node, "i8");
    ASSERT_TRUE(res->m_types[0].m_bitSize.has_value());
    EXPECT_EQ(res->m_types[0].m_bitSize->m_node, 8);

    EXPECT_EQ(res->m_types[1].m_kind, DSL::Ast::TypeDef::TypeKind::Integer);
    EXPECT_EQ(res->m_types[1].m_name.m_node, "i16");
    ASSERT_TRUE(res->m_types[1].m_bitSize.has_value());
    EXPECT_EQ(res->m_types[1].m_bitSize->m_node, 16);

    EXPECT_EQ(res->m_types[2].m_kind, DSL::Ast::TypeDef::TypeKind::Integer);
    EXPECT_EQ(res->m_types[2].m_name.m_node, "i32");
    ASSERT_TRUE(res->m_types[2].m_bitSize.has_value());
    EXPECT_EQ(res->m_types[2].m_bitSize->m_node, 32);

    EXPECT_EQ(res->m_types[3].m_kind, DSL::Ast::TypeDef::TypeKind::Integer);
    EXPECT_EQ(res->m_types[3].m_name.m_node, "i64");
    ASSERT_TRUE(res->m_types[3].m_bitSize.has_value());
    EXPECT_EQ(res->m_types[3].m_bitSize->m_node, 64);

    EXPECT_EQ(res->m_types[4].m_kind, DSL::Ast::TypeDef::TypeKind::FloatingPoint);
    EXPECT_EQ(res->m_types[4].m_name.m_node, "f32");
    ASSERT_TRUE(res->m_types[4].m_bitSize.has_value());
    EXPECT_EQ(res->m_types[4].m_bitSize->m_node, 32);

    EXPECT_EQ(res->m_types[5].m_kind, DSL::Ast::TypeDef::TypeKind::FloatingPoint);
    EXPECT_EQ(res->m_types[5].m_name.m_node, "f64");
    ASSERT_TRUE(res->m_types[5].m_bitSize.has_value());
    EXPECT_EQ(res->m_types[5].m_bitSize->m_node, 64);

    EXPECT_EQ(res->m_types[6].m_kind, DSL::Ast::TypeDef::TypeKind::Void);
    EXPECT_EQ(res->m_types[6].m_name.m_node, "void");
    EXPECT_FALSE(res->m_types[6].m_bitSize.has_value());

    EXPECT_EQ(res->m_types[7].m_kind, DSL::Ast::TypeDef::TypeKind::BindingToken);
    EXPECT_EQ(res->m_types[7].m_name.m_node, "__bindToken");
    EXPECT_FALSE(res->m_types[7].m_bitSize.has_value());
}

// ============================================================================
// 3. Negative & Error Parsing Tests
// ============================================================================

/**
 * Verifies syntax error rejection on unsupported type kind keywords (e.g. double).
 */
TEST_F(TypeDefLangTest, TestUnknownTypeKindFails)
{
    std::string test = "double d64(64);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when a type declaration statement is missing a terminating semicolon.
 */
TEST_F(TypeDefLangTest, TestMissingSemicolonInFileFails)
{
    std::string test = "integer i32(32)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when bit size parameter is missing parentheses.
 */
TEST_F(TypeDefLangTest, TestMissingBitSizeParenthesesFails)
{
    std::string test = "integer i32 32;";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    EXPECT_FALSE(res.has_value());
}

/**
 * Verifies syntax error rejection when type name identifier is omitted.
 */
TEST_F(TypeDefLangTest, TestMissingTypeNameFails)
{
    std::string test = "integer (32);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    EXPECT_FALSE(res.has_value());
}