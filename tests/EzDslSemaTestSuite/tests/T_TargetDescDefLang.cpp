#include "EzDslSemaTestSuite.h"
#include "Ast/TargetDescDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDescDefLang.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetDescSymbols.h"
#include "SemaPasses/TargetDescPass.h"

using namespace DSL;

/**
 * Test fixture for the .tdesc target descriptor semantic pass.
 */
class TargetDescPassTest : public EzDslSemaTestSuiteAsGtest
{
  protected:
    bool parseAndRun(const std::string &sourceContent)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.tdesc", m_currentTestId++), sourceContent);
        m_ast = ctx.parse<Parser::TargetDesc::TargetDescFileParser, Ast::TargetDesc::TargetDescFile>();
        if (!m_ast.has_value())
        {
            return false;
        }

        TargetDescPass pass;
        return pass.run(getDiagCollector(), getSymbolTable(), &m_ast.value());
    }

    std::optional<Ast::TargetDesc::TargetDescFile> m_ast;

  private:
    size_t m_currentTestId{ 0 };
};

TEST_F(TargetDescPassTest, AcceptsValidManifestAndDeclaresSymbol)
{
    std::string source = R"(
target X86_64 {
    registers: "x86_64_registers.reg";
    pointer_size: 8;
    stack_slot: 8;
    instruction_pointer: rip;
    object_formats: [ELF, COFF];
    default_calling_conv: SysV_AMD64;
    libcalls {
        __returnNothing: "__returnNothing";
        __divdi3: "__divdi3"
    }
    components {
        frame_lowerer: X86_64FrameLowerer;
        register_allocator: X86_64RegisterAllocator
    }
}
)";

    EXPECT_TRUE(parseAndRun(source));
    auto *sym = getSymbolTable()->getSymByName("X86_64", SymbolType::TargetDesc);
    ASSERT_NE(sym, nullptr);
    EXPECT_TRUE(sym->hasData<Symbols::TargetDescSymbol>());
    EXPECT_EQ(sym->getIf<Symbols::TargetDescSymbol>()->m_name, "X86_64");
}

TEST_F(TargetDescPassTest, RejectsNonPositivePointerSize)
{
    std::string source = R"(
target Bad {
    pointer_size: 0;
    object_formats: [ELF];
    default_calling_conv: C
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(TargetDescPassTest, RejectsNonPowerOfTwoStackSlot)
{
    std::string source = R"(
target Bad {
    pointer_size: 8;
    stack_slot: 12;
    object_formats: [ELF];
    default_calling_conv: C
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(TargetDescPassTest, RejectsDuplicateComponentSlot)
{
    std::string source = R"(
target Bad {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;
    components {
        frame_lowerer: A;
        frame_lowerer: B
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(TargetDescPassTest, RejectsDuplicateLibcallId)
{
    std::string source = R"(
target Bad {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C;
    libcalls {
        __divdi3: "__divdi3";
        __divdi3: "__udivdi3"
    }
}
)";
    EXPECT_FALSE(parseAndRun(source));
}

TEST_F(TargetDescPassTest, RejectsDuplicateTargetSymbol)
{
    std::string source = R"(
target Dup {
    pointer_size: 8;
    stack_slot: 8;
    object_formats: [ELF];
    default_calling_conv: C
}
)";

    EXPECT_TRUE(parseAndRun(source));

    ParseContext ctx = createParseContextFromBuff("dup_second.tdesc", source);
    auto ast = ctx.parse<Parser::TargetDesc::TargetDescFileParser, Ast::TargetDesc::TargetDescFile>();
    ASSERT_TRUE(ast.has_value());

    TargetDescPass pass;
    EXPECT_FALSE(pass.run(getDiagCollector(), getSymbolTable(), &ast.value()));
}
