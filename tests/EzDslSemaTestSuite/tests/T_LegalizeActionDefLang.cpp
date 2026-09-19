#include "EzDslSemaTestSuite.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeActionPass.h"

/**
 * Fixture for the legalize-action semantic pass with standard types and IR instructions predeclared.
 */
class LegalizeActionPassTest : public EzDslSemaTestSuiteAsGtest
{
  protected:
    // Declares the standard types and IR instructions used by the legalize-action tests.
    void SetUp() override
    {
        EzDslSemaTestSuiteAsGtest::SetUp();
        declareStandardTypes();
        declareStandardIrInstructions();
    }

    // Registers the primitive integer, float, and pointer types in the symbol table.
    void declareStandardTypes()
    {
        auto declareType = [&](std::string_view name, DSL::Ast::TypeDef::TypeKind kind, uint32_t bitWidth)
        {
            Symbols::TypeSymbol symData{ .m_name = name,
                                         .m_kind = kind,
                                         .m_bitWidth = bitWidth,
                                         .m_alignment = bitWidth,
                                         .m_compactId = 0 };
            getSymbolTable()->declareSym(nullptr, SymbolType::Type, std::move(symData), name);
        };

        declareType("i1", DSL::Ast::TypeDef::TypeKind::Integer, 1);
        declareType("i8", DSL::Ast::TypeDef::TypeKind::Integer, 8);
        declareType("i16", DSL::Ast::TypeDef::TypeKind::Integer, 16);
        declareType("i32", DSL::Ast::TypeDef::TypeKind::Integer, 32);
        declareType("i64", DSL::Ast::TypeDef::TypeKind::Integer, 64);
        declareType("i128", DSL::Ast::TypeDef::TypeKind::Integer, 128);
        declareType("f32", DSL::Ast::TypeDef::TypeKind::FloatingPoint, 32);
        declareType("f64", DSL::Ast::TypeDef::TypeKind::FloatingPoint, 64);
        declareType("ptr", DSL::Ast::TypeDef::TypeKind::Pointer, 64);
    }

    // Registers a standard set of IR instruction symbols used by the test sources.
    void declareStandardIrInstructions()
    {
        auto declareInst = [&](std::string_view name)
        {
            Symbols::IrInstructionSymbol symData{ .m_name = name,
                                                  .m_category = DSL::Ast::IrInstDef::IrInstCategory::Arithmetic,
                                                  .m_tier = DSL::Ast::IrInstDef::IrInstTier::HighLevel,
                                                  .m_flags = DSL::Ast::IrInstDef::IrInstFlag::None,
                                                  .m_operands = std::pmr::vector<Symbols::IrOperandSymbol>{
                                                          getSymbolTable()->getAllocator() } };
            getSymbolTable()->declareSym(nullptr, SymbolType::IrInstruction, std::move(symData), name);
        };

        declareInst("ADD");
        declareInst("SUB");
        declareInst("AND");
        declareInst("OR");
        declareInst("XOR");
        declareInst("SDIV");
        declareInst("SEXT");
        declareInst("STORE");
        declareInst("CALL");
        declareInst("RET");
        declareInst("ALLOC");
    }

    // Parses a legalize-action source into an AST using a unique .lad source name.
    std::optional<DSL::Ast::LegalizeActionDef::LegalizeActionFile> parseFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.lad", m_testId++), source);
        return ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeActionFile,
                         DSL::Ast::LegalizeActionDef::LegalizeActionFile>();
    }

  private:
    size_t m_testId{ 0 };
};

// Verifies a type_set declaration registers a TypeSet symbol with its member types.
TEST_F(LegalizeActionPassTest, TestTypeSetRegistration)
{
    std::string source = R"(
        type_set GPR_SCALARS = (i8, i16, i32, i64);
    )";

    auto ast = parseFile(source);
    ASSERT_TRUE(ast.has_value());

    bool success = LegalizeActionPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_TRUE(success);

    Symbol *tsSym = getSymbolTable()->getSymByName("GPR_SCALARS");
    ASSERT_NE(tsSym, nullptr);
    EXPECT_EQ(tsSym->getType(), SymbolType::TypeSet);

    const auto *tsData = tsSym->getIf<Symbols::TypeSetSymbol>();
    ASSERT_NE(tsData, nullptr);
    EXPECT_EQ(tsData->m_name, "GPR_SCALARS");
    EXPECT_EQ(tsData->m_typeIds.size(), 4);
}

// Verifies CLAMP_SCALAR expands into widen/legal/narrow clauses for the type range.
TEST_F(LegalizeActionPassTest, TestClampScalarExpansion)
{
    std::string source = R"(
        action ADD {
            CLAMP_SCALAR(i32, i64);
        };
    )";

    auto ast = parseFile(source);
    ASSERT_TRUE(ast.has_value());

    bool success = LegalizeActionPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_TRUE(success);

    Symbol *actSym = nullptr;
    for (Symbol *s : getSymbolTable()->getSymbols())
    {
        if (s && s->getType() == SymbolType::LegalizeAction && s->getName() == "ADD")
        {
            actSym = s;
            break;
        }
    }
    ASSERT_NE(actSym, nullptr);
    const auto *actData = actSym->getIf<Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actData, nullptr);

    // Should contain clauses for:
    // i1 (< 32): WidenScalar to i32
    // i8 (< 32): WidenScalar to i32
    // i16 (< 32): WidenScalar to i32
    // i32 (32 <= 32 <= 64): Legal
    // i64 (32 <= 64 <= 64): Legal
    // i128 (> 64): NarrowScalar to i64
    EXPECT_GE(actData->m_clauses.size(), 6);

    size_t widenCount = 0;
    size_t legalCount = 0;
    size_t narrowCount = 0;

    for (const auto &clause : actData->m_clauses)
    {
        if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar)
        {
            ++widenCount;
        }
        else if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::Legal)
        {
            ++legalCount;
        }
        else if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar)
        {
            ++narrowCount;
        }
    }

    EXPECT_EQ(widenCount, 3);
    EXPECT_EQ(legalCount, 2);
    EXPECT_EQ(narrowCount, 1);
}

// Verifies a clamp with min greater than max is rejected.
TEST_F(LegalizeActionPassTest, TestInvalidClampRangeError)
{
    std::string source = R"(
        action ADD {
            CLAMP_SCALAR(i64, i32);
        };
    )";

    auto ast = parseFile(source);
    ASSERT_TRUE(ast.has_value());

    bool success = LegalizeActionPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_FALSE(success);
}

// Verifies an instruction group expands its clauses to each member instruction.
TEST_F(LegalizeActionPassTest, TestGroupExpansion)
{
    std::string source = R"(
        group IntegerALU = (SUB, AND, OR, XOR) {
            CLAMP_SCALAR(i32, i64);
        };
    )";

    auto ast = parseFile(source);
    ASSERT_TRUE(ast.has_value());

    bool success = LegalizeActionPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_TRUE(success);

    for (std::string_view op : { "SUB", "AND", "OR", "XOR" })
    {
        Symbol *actSym = nullptr;
        for (Symbol *s : getSymbolTable()->getSymbols())
        {
            if (s && s->getType() == SymbolType::LegalizeAction && s->getName() == op)
            {
                actSym = s;
                break;
            }
        }
        ASSERT_NE(actSym, nullptr);
        const auto *actData = actSym->getIf<Symbols::LegalizeActionSymbol>();
        ASSERT_NE(actData, nullptr);
        EXPECT_FALSE(actData->m_clauses.empty());
    }
}

// Verifies a LOWER clause records its named lowering handler on the action symbol.
TEST_F(LegalizeActionPassTest, TestLowerAction)
{
    std::string source = R"(
        action CALL {
            LOWER >> AMD64CallLowering;
        };
    )";

    auto ast = parseFile(source);
    ASSERT_TRUE(ast.has_value());

    bool success = LegalizeActionPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_TRUE(success);

    Symbol *actSym = nullptr;
    for (Symbol *s : getSymbolTable()->getSymbols())
    {
        if (s && s->getType() == SymbolType::LegalizeAction && s->getName() == "CALL")
        {
            actSym = s;
            break;
        }
    }
    ASSERT_NE(actSym, nullptr);
    const auto *actData = actSym->getIf<Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actData, nullptr);
    ASSERT_EQ(actData->m_clauses.size(), 1);
    EXPECT_EQ(actData->m_clauses[0].m_kind, DSL::Ast::LegalizeActionDef::LegalizeActionKind::Lower);
    ASSERT_TRUE(actData->m_clauses[0].m_lowerHandler.has_value());
    EXPECT_EQ(*actData->m_clauses[0].m_lowerHandler, "AMD64CallLowering");
}

// Verifies a type_set in an indexed constraint expands into the Cartesian product of LEGAL clauses.
TEST_F(LegalizeActionPassTest, TestHeterogeneousTypeSetExpansion)
{
    std::string source = R"(
        type_set GPR_SCALARS = (i8, i16, i32, i64);

        action STORE {
            LEGAL(GPR_SCALARS:0, ptr:1);
            WIDENS(i1:0) >> i8;
        };
    )";

    auto ast = parseFile(source);
    ASSERT_TRUE(ast.has_value());

    bool success = LegalizeActionPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    EXPECT_TRUE(success);

    Symbol *actSym = nullptr;
    for (Symbol *s : getSymbolTable()->getSymbols())
    {
        if (s && s->getType() == SymbolType::LegalizeAction && s->getName() == "STORE")
        {
            actSym = s;
            break;
        }
    }
    ASSERT_NE(actSym, nullptr);
    const auto *actData = actSym->getIf<Symbols::LegalizeActionSymbol>();
    ASSERT_NE(actData, nullptr);

    // 4 Cartesian product LEGAL clauses (i8, i16, i32, i64) + 1 WIDENS clause = 5 clauses
    EXPECT_EQ(actData->m_clauses.size(), 5);
}
