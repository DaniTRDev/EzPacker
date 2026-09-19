#include "EzDslLexerTestSuite.h"
#include "Ast/RegisterDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/RegisterDefLang.h"

using namespace DSL;

/**
 * Test fixture for the .reg register definition dialect parser.
 */
class RegisterDefLangTest : public DslLexerTestSuiteAsGtest
{
  protected:
    std::optional<Ast::RegisterDef::RegisterFile> parse(const std::string &sourceContent)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.reg", m_currentTestId++), sourceContent);
        return ctx.parse<Parser::RegisterDef::RegisterDefFile, Ast::RegisterDef::RegisterFile>();
    }

  private:
    size_t m_currentTestId{ 0 };
};

TEST_F(RegisterDefLangTest, ParsesRegisterBanksClassesAndAliases)
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

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());

    EXPECT_EQ(ast->m_target.m_node, "X86_64");
    ASSERT_EQ(ast->m_banks.size(), 1u);

    const auto &bank = ast->m_banks[0];
    EXPECT_EQ(bank.m_name.m_node, "GPR");
    ASSERT_EQ(bank.m_classes.size(), 4u);
    EXPECT_EQ(bank.m_classes[0].m_name.m_node, "GPR8");
    EXPECT_EQ(bank.m_classes[0].m_bitSize.m_node, 8);
    EXPECT_EQ(bank.m_classes[3].m_name.m_node, "GPR64");
    EXPECT_EQ(bank.m_classes[3].m_bitSize.m_node, 64);

    ASSERT_EQ(bank.m_subRegisterEdges.size(), 3u);
    EXPECT_EQ(bank.m_subRegisterEdges[0].m_wideClass.m_node, "GPR16");
    EXPECT_EQ(bank.m_subRegisterEdges[0].m_narrowClass.m_node, "GPR8");
    EXPECT_EQ(bank.m_subRegisterEdges[2].m_wideClass.m_node, "GPR64");

    ASSERT_EQ(bank.m_registers.size(), 2u);
    const auto &rax = bank.m_registers[0];
    EXPECT_EQ(rax.m_canonicalName.m_node, "rax");
    EXPECT_EQ(rax.m_encoding.m_node, 0);
    ASSERT_EQ(rax.m_names.size(), 4u);
    EXPECT_EQ(rax.m_names[0].m_asmName.m_node, "rax");
    EXPECT_EQ(rax.m_names[0].m_className.m_node, "GPR64");
    EXPECT_EQ(rax.m_names[3].m_asmName.m_node, "al");
    EXPECT_EQ(rax.m_names[3].m_className.m_node, "GPR8");

    const auto &rcx = bank.m_registers[1];
    EXPECT_EQ(rcx.m_encoding.m_node, 1);
    EXPECT_EQ(rcx.m_names[0].m_asmName.m_node, "rcx");

    ASSERT_EQ(ast->m_specialRegs.size(), 1u);
    EXPECT_EQ(ast->m_specialRegs[0].m_name.m_node, "rip");
    EXPECT_EQ(ast->m_specialRegs[0].m_id.m_node, 16);
}

TEST_F(RegisterDefLangTest, SubRegisterBlockIsOptional)
{
    std::string source = R"(
target Foo;

register_bank FPR {
    classes { FPR32: 32, FPR64: 64 }
    registers {
        xmm0 enc 0 names { xmm0: FPR32, xmm0: FPR64 }
    }
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());
    ASSERT_EQ(ast->m_banks.size(), 1u);
    EXPECT_TRUE(ast->m_banks[0].m_subRegisterEdges.empty());
    ASSERT_EQ(ast->m_banks[0].m_registers.size(), 1u);
    EXPECT_EQ(ast->m_banks[0].m_registers[0].m_names.size(), 2u);
}

TEST_F(RegisterDefLangTest, ParsesMultipleBanksAndOptionalSemicolon)
{
    std::string source = R"(
target Foo;

register_bank A {
    classes { A8: 8 }
    registers { a0 enc 0 names { a0: A8 } }
}

register_bank B {
    classes { B8: 8 }
    registers { b0 enc 0 names { b0: B8 } }
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());
    ASSERT_EQ(ast->m_banks.size(), 2u);
    EXPECT_EQ(ast->m_banks[0].m_name.m_node, "A");
    EXPECT_EQ(ast->m_banks[1].m_name.m_node, "B");
    EXPECT_EQ(ast->m_target.m_node, "Foo");
}

TEST_F(RegisterDefLangTest, RejectsMissingTarget)
{
    auto ast = parse("register_bank GPR { classes { G8: 8 } registers { r0 enc 0 names { r0: G8 } } }");
    EXPECT_FALSE(ast.has_value());
}
