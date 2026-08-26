#include "EzDslTestSuite.h"
#include "Ast/TargetDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/TargetDefPass.h"

/**
 * Test fixture for target architecture declaration semantic pass (TargetDefPass).
 * Verifies declaration, duplication rejection, symbol collision checks, and null safety.
 */
class TargetDefPassTest : public DslTestSuiteAsGtest
{
  protected:
    std::optional<DSL::Ast::TargetDef::TargetDef> parseTarget(const std::string &content)
    {
        ParseContext parseCtx = createParseContextFromBuff("target_test.tdf", content);
        return parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    }
};

/**
 * Verifies successful declaration of target architecture symbol in SymbolTable.
 */
TEST_F(TargetDefPassTest, DeclaresTargetArchitectureSymbol)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64, rax(, 64, 0));
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_TRUE(TargetDefPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *targetSym = table.getSymByName("x86_64");
    ASSERT_NE(targetSym, nullptr);
    EXPECT_EQ(targetSym->getType(), SymbolType::Target);
    EXPECT_TRUE(targetSym->hasFlag(SymbolFlags::IsDefined));

    const auto *targetData = targetSym->getIf<Sema::Symbols::TargetSymbol>();
    ASSERT_NE(targetData, nullptr);
    EXPECT_EQ(targetData->m_name, "x86_64");
}

/**
 * Verifies that redefinition of a target architecture fails.
 */
TEST_F(TargetDefPassTest, FailsOnDuplicateTargetDeclaration)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64, rax(, 64, 0));
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_TRUE(TargetDefPass::run(getDiagCollector(), &table, &*ast));
    EXPECT_FALSE(TargetDefPass::run(getDiagCollector(), &table, &*ast));
}

/**
 * Verifies that TargetDefPass safely handles null inputs.
 */
TEST_F(TargetDefPassTest, FailsGracefullyOnNullInputs)
{
    DiagnosticCollector collector;
    SymbolTable table(getAllocator());

    EXPECT_FALSE(TargetDefPass::run(&collector, nullptr, nullptr));
    EXPECT_FALSE(TargetDefPass::run(nullptr, &table, nullptr));
}
