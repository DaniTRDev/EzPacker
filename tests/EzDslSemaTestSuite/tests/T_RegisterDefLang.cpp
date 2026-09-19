#include "EzDslSemaTestSuite.h"
#include "Ast/RegisterDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/RegisterDefLang.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/RegisterSymbols.h"
#include "SemaPasses/RegisterPass.h"

using namespace DSL;

/**
 * Test fixture for the .reg register definition semantic pass.
 */
class RegisterPassTest : public EzDslSemaTestSuiteAsGtest
{
  protected:
    bool parseAndRun(const std::string &sourceContent)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.reg", m_currentTestId++), sourceContent);
        auto ast = ctx.parse<Parser::RegisterDef::RegisterDefFile, Ast::RegisterDef::RegisterFile>();
        if (!ast.has_value())
        {
            return false;
        }

        RegisterPass pass;
        return pass.run(getDiagCollector(), getSymbolTable(), &ast.value());
    }

  private:
    size_t m_currentTestId{ 0 };
};

TEST_F(RegisterPassTest, AcceptsValidX86LikeRegisterFile)
{
    std::string source = R"(
target X86_64;

register_bank GPR {
    classes { GPR8: 8, GPR16: 16, GPR32: 32, GPR64: 64 }
    sub_register { GPR16 <: GPR8, GPR32 <: GPR16, GPR64 <: GPR32 }
    registers {
        rax enc 0  names { rax: GPR64, eax: GPR32, ax: GPR16, al: GPR8 }
        rcx enc 1  names { rcx: GPR64, ecx: GPR32, cx: GPR16, cl: GPR8 }
    }
}

special {
    rip: 16
}
)";

    EXPECT_TRUE(parseAndRun(source));

    auto *fileSym = getSymbolTable()->getSymByName("X86_64", SymbolType::RegisterFile);
    ASSERT_NE(fileSym, nullptr);
    EXPECT_TRUE(fileSym->hasData<Symbols::RegisterFileSymbol>());

    auto *bankSym = getSymbolTable()->getSymByName("GPR", SymbolType::RegisterBank);
    ASSERT_NE(bankSym, nullptr);
    EXPECT_TRUE(bankSym->hasData<Symbols::RegisterBankSymbol>());

    auto *classSym = getSymbolTable()->getSymByName("GPR64", SymbolType::RegisterClass);
    ASSERT_NE(classSym, nullptr);
    const auto *classData = classSym->getIf<Symbols::RegisterClassSymbol>();
    ASSERT_NE(classData, nullptr);
    EXPECT_EQ(classData->m_bitSize, 64u);
    EXPECT_EQ(classData->m_bankName, "GPR");

    auto *regSym = getSymbolTable()->getSymByName("rax", SymbolType::Register);
    ASSERT_NE(regSym, nullptr);
    const auto *regData = regSym->getIf<Symbols::RegisterSymbol>();
    ASSERT_NE(regData, nullptr);
    EXPECT_EQ(regData->m_hwEncoding, 0u);
    EXPECT_EQ(regData->m_bankName, "GPR");

    auto *specialSym = getSymbolTable()->getSymByName("rip", SymbolType::SpecialRegister);
    ASSERT_NE(specialSym, nullptr);
    EXPECT_EQ(specialSym->getIf<Symbols::SpecialRegisterSymbol>()->m_id, 16u);
}

TEST_F(RegisterPassTest, RejectsDuplicateBankNames)
{
    std::string source = R"(
target Foo;
register_bank GPR { classes { G8: 8 } registers { r0 enc 0 names { r0: G8 } } }
register_bank GPR { classes { G8: 8 } registers { r1 enc 1 names { r1: G8 } } }
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(RegisterPassTest, RejectsDuplicateEncodingWithinBank)
{
    std::string source = R"(
target Foo;
register_bank GPR {
    classes { G8: 8 }
    registers {
        r0 enc 0 names { r0: G8 }
        r1 enc 0 names { r1: G8 }
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(RegisterPassTest, RejectsSubRegisterEdgeToUndeclaredClass)
{
    std::string source = R"(
target Foo;
register_bank GPR {
    classes { G8: 8 }
    sub_register { G16 <: G8 }
    registers { r0 enc 0 names { r0: G8 } }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(RegisterPassTest, RejectsNonNarrowingSubRegisterEdge)
{
    std::string source = R"(
target Foo;
register_bank GPR {
    classes { G8: 8, G16: 16 }
    sub_register { G8 <: G16 }
    registers { r0 enc 0 names { r0: G8 } }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(RegisterPassTest, RejectsNameBindingToUndeclaredClass)
{
    std::string source = R"(
target Foo;
register_bank GPR {
    classes { G8: 8 }
    registers { r0 enc 0 names { r0: G64 } }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(RegisterPassTest, RejectsSpecialRegisterIdCollision)
{
    std::string source = R"(
target Foo;
register_bank GPR {
    classes { G8: 8 }
    registers { r0 enc 0 names { r0: G8 } }
}
special { rip: 0 }
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(RegisterPassTest, RejectsDuplicateClassAcrossBanks)
{
    std::string source = R"(
target Foo;
register_bank A { classes { G8: 8 } registers { a0 enc 0 names { a0: G8 } } }
register_bank B { classes { G8: 8 } registers { b0 enc 0 names { b0: G8 } } }
)";
    EXPECT_FALSE(parseAndRun(source));
}
