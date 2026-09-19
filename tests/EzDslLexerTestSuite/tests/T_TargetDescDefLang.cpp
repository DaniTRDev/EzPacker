#include "EzDslLexerTestSuite.h"
#include "Ast/TargetDescDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDescDefLang.h"

using namespace DSL;

/**
 * Test fixture for the .tdesc target descriptor dialect parser.
 */
class TargetDescDefLangTest : public DslLexerTestSuiteAsGtest
{
  protected:
    /**
     * Parses target descriptor source into an AST, using a unique source name per call.
     */
    std::optional<Ast::TargetDesc::TargetDescFile> parse(const std::string &sourceContent)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.tdesc", m_currentTestId++), sourceContent);
        return ctx.parse<Parser::TargetDesc::TargetDescFileParser, Ast::TargetDesc::TargetDescFile>();
    }

  private:
    size_t m_currentTestId{ 0 };
};

/**
 * Verifies a complete target descriptor parses all fields: file references,
 * sizes, instruction pointer, memory displacement type, object formats,
 * default calling convention, libcalls, and components.
 */
TEST_F(TargetDescDefLangTest, ParsesFullManifest)
{
    std::string source = R"(
target X86_64 {
    registers:     "x86_64_registers.reg";
    instructions:  "x86_64_instructions.idf";
    calling_convs: ["x86_64_calling_conv.ezcc"];

    pointer_size: 8;
    stack_slot:   8;

    instruction_pointer: rip;
    mem_disp_type: i64;

    object_formats: [ELF, COFF];
    default_calling_conv: SysV_AMD64;

    libcalls {
        __returnNothing: "__returnNothing";
        __divdi3:        "__divdi3"
    }

    components {
        frame_lowerer:        X86_64FrameLowerer;
        instruction_selector: X86_64TargetInstructionSelector
    }
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());

    EXPECT_EQ(ast->m_name.m_node, "X86_64");
    ASSERT_TRUE(ast->m_registers.has_value());
    EXPECT_EQ(ast->m_registers->m_node, "x86_64_registers.reg");
    ASSERT_TRUE(ast->m_instructions.has_value());
    EXPECT_EQ(ast->m_instructions->m_node, "x86_64_instructions.idf");
    ASSERT_EQ(ast->m_callingConvs.size(), 1u);
    EXPECT_EQ(ast->m_callingConvs[0].m_node, "x86_64_calling_conv.ezcc");

    ASSERT_TRUE(ast->m_pointerSize.has_value());
    EXPECT_EQ(ast->m_pointerSize->m_node, 8);
    ASSERT_TRUE(ast->m_stackSlot.has_value());
    EXPECT_EQ(ast->m_stackSlot->m_node, 8);

    ASSERT_TRUE(ast->mInstructionPointer.has_value());
    EXPECT_EQ(ast->mInstructionPointer->m_node, "rip");
    ASSERT_TRUE(ast->mMemDispType.has_value());
    EXPECT_EQ(ast->mMemDispType->m_node, "i64");

    ASSERT_EQ(ast->mObjectFormats.size(), 2u);
    EXPECT_EQ(ast->mObjectFormats[0].m_node, "ELF");
    EXPECT_EQ(ast->mObjectFormats[1].m_node, "COFF");
    ASSERT_TRUE(ast->mDefaultCallingConv.has_value());
    EXPECT_EQ(ast->mDefaultCallingConv->m_node, "SysV_AMD64");

    ASSERT_EQ(ast->mLibcalls.size(), 2u);
    EXPECT_EQ(ast->mLibcalls[0].m_name.m_node, "__returnNothing");
    EXPECT_EQ(ast->mLibcalls[0].m_symbol.m_node, "__returnNothing");
    EXPECT_EQ(ast->mLibcalls[1].m_name.m_node, "__divdi3");

    ASSERT_EQ(ast->mComponents.size(), 2u);
    EXPECT_EQ(ast->mComponents[0].m_slot.m_node, "frame_lowerer");
    EXPECT_EQ(ast->mComponents[0].m_type.m_node, "X86_64FrameLowerer");
    EXPECT_EQ(ast->mComponents[1].m_slot.m_node, "instruction_selector");
}

/**
 * Verifies the minimal set of required fields parses, leaving optional
 * file references, libcalls, and components absent.
 */
TEST_F(TargetDescDefLangTest, ParsesMinimalManifest)
{
    std::string source = R"(
target AArch64 {
    pointer_size: 8;
    stack_slot: 16;
    object_formats: [ELF];
    default_calling_conv: AAPCS
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());
    EXPECT_EQ(ast->m_name.m_node, "AArch64");
    EXPECT_FALSE(ast->m_registers.has_value());
    EXPECT_TRUE(ast->mLibcalls.empty());
    EXPECT_TRUE(ast->mComponents.empty());
}

/**
 * Verifies parsing fails when the leading `target` keyword is omitted.
 */
TEST_F(TargetDescDefLangTest, RejectsMissingTargetKeyword)
{
    auto ast = parse("X86_64 { pointer_size: 8; }");
    EXPECT_FALSE(ast.has_value());
}
