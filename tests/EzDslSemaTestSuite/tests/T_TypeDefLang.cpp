#include "EzDslSemaTestSuite.h"
#include "Ast/TypeDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TypeDefLang.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/TypePass.h"

/**
 * Test fixture for semantic analysis of Type Definition Language (.tyf) AST.
 */
class TypePassTest : public EzDslSemaTestSuiteAsGtest
{
  protected:
    std::optional<DSL::Ast::TypeDef::TypeDefFile> parseTypeDefFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.tyf", m_currentTestId++), source);
        return ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    }

  private:
    size_t m_currentTestId{ 0 };
};

// ============================================================================
// 1. Successful Semantic Registration & Defaults
// ============================================================================

/**
 * Verifies valid declaration and semantic registration of integer, float, void,
 * binding token, and pointer types, including default alignment fallback.
 */
TEST_F(TypePassTest, TestSuccessfulTypeDeclarationsAndDefaults)
{
    std::string source = R"(
        integer i32(32);
        float f64(64);
        void v();
        bindingToken token();
        pointer ptr();
    )";

    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    bool success = pass.run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_TRUE(success);

    // Verify 'i32' symbol
    Symbol *i32Sym = getSymbolTable()->getSymByName("i32");
    ASSERT_NE(i32Sym, nullptr);
    EXPECT_EQ(i32Sym->getType(), SymbolType::Type);
    auto *i32Data = i32Sym->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(i32Data, nullptr);
    EXPECT_EQ(i32Data->m_name, "i32");
    EXPECT_EQ(i32Data->m_kind, DSL::Ast::TypeDef::TypeKind::Integer);
    EXPECT_EQ(i32Data->m_bitWidth, 32);
    EXPECT_EQ(i32Data->m_alignment, 32); // Defaulted to bitWidth
    EXPECT_EQ(i32Data->m_compactId, 1);

    // Verify 'f64' symbol
    Symbol *f64Sym = getSymbolTable()->getSymByName("f64");
    ASSERT_NE(f64Sym, nullptr);
    auto *f64Data = f64Sym->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(f64Data, nullptr);
    EXPECT_EQ(f64Data->m_kind, DSL::Ast::TypeDef::TypeKind::FloatingPoint);
    EXPECT_EQ(f64Data->m_bitWidth, 64);
    EXPECT_EQ(f64Data->m_alignment, 64); // Defaulted to bitWidth
    EXPECT_EQ(f64Data->m_compactId, 2);

    // Verify 'v' void symbol
    Symbol *voidSym = getSymbolTable()->getSymByName("v");
    ASSERT_NE(voidSym, nullptr);
    auto *voidData = voidSym->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(voidData, nullptr);
    EXPECT_EQ(voidData->m_kind, DSL::Ast::TypeDef::TypeKind::Void);
    EXPECT_EQ(voidData->m_bitWidth, 0);
    EXPECT_EQ(voidData->m_alignment, 0);
    EXPECT_EQ(voidData->m_compactId, 3);

    // Verify 'token' binding token symbol
    Symbol *tokSym = getSymbolTable()->getSymByName("token");
    ASSERT_NE(tokSym, nullptr);
    auto *tokData = tokSym->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(tokData, nullptr);
    EXPECT_EQ(tokData->m_kind, DSL::Ast::TypeDef::TypeKind::BindingToken);
    EXPECT_EQ(tokData->m_bitWidth, 0);
    EXPECT_EQ(tokData->m_alignment, 0);
    EXPECT_EQ(tokData->m_compactId, 4);

    // Verify 'ptr' pointer symbol
    Symbol *ptrSym = getSymbolTable()->getSymByName("ptr");
    ASSERT_NE(ptrSym, nullptr);
    auto *ptrData = ptrSym->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(ptrData, nullptr);
    EXPECT_EQ(ptrData->m_kind, DSL::Ast::TypeDef::TypeKind::Pointer);
    EXPECT_EQ(ptrData->m_bitWidth, 0);
    EXPECT_EQ(ptrData->m_alignment, 0);
    EXPECT_EQ(ptrData->m_compactId, 5);
}

/**
 * Verifies explicit custom alignment override on integer and float declarations.
 */
TEST_F(TypePassTest, TestCustomAlignmentOverride)
{
    std::string source = R"(
        integer i32(32, 64);
        float f32(32, 128);
    )";

    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_TRUE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));

    Symbol *i32Sym = getSymbolTable()->getSymByName("i32");
    ASSERT_NE(i32Sym, nullptr);
    auto *i32Data = i32Sym->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(i32Data, nullptr);
    EXPECT_EQ(i32Data->m_bitWidth, 32);
    EXPECT_EQ(i32Data->m_alignment, 64);

    Symbol *f32Sym = getSymbolTable()->getSymByName("f32");
    ASSERT_NE(f32Sym, nullptr);
    auto *f32Data = f32Sym->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(f32Data, nullptr);
    EXPECT_EQ(f32Data->m_bitWidth, 32);
    EXPECT_EQ(f32Data->m_alignment, 128);
}

/**
 * Verifies boundary-valid bit widths for integers (1-bit and 1024-bit).
 */
TEST_F(TypePassTest, TestBoundaryIntegerBitWidths)
{
    std::string source = R"(
        integer i1(1);
        integer i1024(1024);
    )";

    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_TRUE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));

    auto *i1 = getSymbolTable()->getSymByName("i1")->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(i1, nullptr);
    EXPECT_EQ(i1->m_bitWidth, 1);

    auto *i1024 = getSymbolTable()->getSymByName("i1024")->getIf<Symbols::TypeSymbol>();
    ASSERT_NE(i1024, nullptr);
    EXPECT_EQ(i1024->m_bitWidth, 1024);
}

/**
 * Verifies all supported floating point bit widths (16, 32, 64, 128).
 */
TEST_F(TypePassTest, TestSupportedFloatBitWidths)
{
    std::string source = R"(
        float f16(16);
        float f32(32);
        float f64(64);
        float f128(128);
    )";

    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_TRUE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
    EXPECT_NE(getSymbolTable()->getSymByName("f16"), nullptr);
    EXPECT_NE(getSymbolTable()->getSymByName("f32"), nullptr);
    EXPECT_NE(getSymbolTable()->getSymByName("f64"), nullptr);
    EXPECT_NE(getSymbolTable()->getSymByName("f128"), nullptr);
}

// ============================================================================
// 2. Semantic Constraints & Error Diagnostics
// ============================================================================

/**
 * Verifies rejection when an integer type declaration omits bit width.
 */
TEST_F(TypePassTest, TestIntegerMissingBitSizeFails)
{
    std::string source = "integer iBad();";
    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
    EXPECT_EQ(getSymbolTable()->getSymByName("iBad"), nullptr);
}

/**
 * Verifies rejection when an integer bit width is zero or greater than 1024.
 */
TEST_F(TypePassTest, TestIntegerInvalidBitWidthFails)
{
    std::string sourceZero = "integer i0(0);";
    auto astZero = parseTypeDefFile(sourceZero);
    ASSERT_TRUE(astZero.has_value());

    TypePass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &astZero.value()));

    std::string sourceTooLarge = "integer i2048(2048);";
    auto astTooLarge = parseTypeDefFile(sourceTooLarge);
    ASSERT_TRUE(astTooLarge.has_value());

    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &astTooLarge.value()));
}

/**
 * Verifies rejection when a floating-point type declaration omits bit width.
 */
TEST_F(TypePassTest, TestFloatMissingBitSizeFails)
{
    std::string source = "float fBad();";
    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies rejection when an unsupported floating-point bit width is specified.
 */
TEST_F(TypePassTest, TestFloatUnsupportedBitWidthFails)
{
    std::string source = "float f80(80);";
    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
    EXPECT_EQ(getSymbolTable()->getSymByName("f80"), nullptr);
}

/**
 * Verifies rejection when zero-width types (void, bindingToken, pointer) declare a non-zero bit size.
 */
TEST_F(TypePassTest, TestZeroWidthTypeNonZeroBitSizeFails)
{
    std::string source = R"(
        void v(32);
        pointer ptr(64);
        bindingToken token(8);
    )";

    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies rejection when zero-width types specify a non-zero alignment.
 */
TEST_F(TypePassTest, TestZeroWidthTypeNonZeroAlignmentFails)
{
    std::string source = "void v(0, 8);";
    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies error handling when a type symbol name collides with an existing symbol.
 */
TEST_F(TypePassTest, TestDuplicateTypeDeclarationFails)
{
    std::string source = R"(
        integer i32(32);
        integer i32(32);
    )";

    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

/**
 * Verifies premature termination when registering more than 254 compact IDs.
 */
TEST_F(TypePassTest, TestCompactIdLimitExceededFails)
{
    std::string source;
    source.reserve(255 * 32);

    for (int i = 0; i < 255; ++i)
    {
        source += "integer t" + std::to_string(i) + "(32);\n";
    }

    auto ast = parseTypeDefFile(source);
    ASSERT_TRUE(ast.has_value());

    TypePass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}

// ============================================================================
// 3. Null Safety Tests
// ============================================================================

/**
 * Verifies safe failure when TypePass receives nullptr inputs.
 */
TEST_F(TypePassTest, TestNullptrArguments)
{
    TypePass pass;
    EXPECT_FALSE(pass.run(nullptr, getSymbolTable(), nullptr));
    EXPECT_FALSE(pass.run(getDiagCollector(), nullptr, nullptr));

    DSL::Ast::TypeDef::TypeDefFile file;
    EXPECT_FALSE(pass.run(getDiagCollector(), nullptr, &file));
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), nullptr));
}