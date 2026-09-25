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

/**
 * Verifies parsing an extension block with default flags, implied dependencies, and descriptions.
 */
TEST_F(TargetDescDefLangTest, ParsesExtensionBlock)
{
    std::string source = R"(
target X86_64 {
    pointer_size: 8;
    stack_slot:   8;
    object_formats: [ELF, COFF];
    default_calling_conv: SysV_AMD64;

    extensions {
        sse {
            default: true;
            description: "Streaming SIMD Extensions";
        };
        sse2 {
            default: true;
            implies: [sse];
        };
        avx {
            implies: [sse2];
            description: "Advanced Vector Extensions";
        };
    }
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());
    ASSERT_EQ(ast->m_extensions.size(), 3u);

    EXPECT_EQ(ast->m_extensions[0].m_name.m_node, "sse");
    ASSERT_TRUE(ast->m_extensions[0].m_default.has_value());
    EXPECT_TRUE(ast->m_extensions[0].m_default->m_node);
    ASSERT_TRUE(ast->m_extensions[0].m_description.has_value());
    EXPECT_EQ(ast->m_extensions[0].m_description->m_node, "Streaming SIMD Extensions");
    EXPECT_TRUE(ast->m_extensions[0].m_implies.empty());

    EXPECT_EQ(ast->m_extensions[1].m_name.m_node, "sse2");
    ASSERT_TRUE(ast->m_extensions[1].m_default.has_value());
    EXPECT_TRUE(ast->m_extensions[1].m_default->m_node);
    ASSERT_EQ(ast->m_extensions[1].m_implies.size(), 1u);
    EXPECT_EQ(ast->m_extensions[1].m_implies[0].m_node, "sse");

    EXPECT_EQ(ast->m_extensions[2].m_name.m_node, "avx");
    EXPECT_FALSE(ast->m_extensions[2].m_default.has_value());
    ASSERT_EQ(ast->m_extensions[2].m_implies.size(), 1u);
    EXPECT_EQ(ast->m_extensions[2].m_implies[0].m_node, "sse2");
    ASSERT_TRUE(ast->m_extensions[2].m_description.has_value());
    EXPECT_EQ(ast->m_extensions[2].m_description->m_node, "Advanced Vector Extensions");
}

/**
 * Verifies parsing the list shorthand for extensions.
 */
TEST_F(TargetDescDefLangTest, ParsesExtensionList)
{
    std::string source = R"(
target X86_64 {
    pointer_size: 8;
    stack_slot:   8;
    object_formats: [ELF];
    default_calling_conv: SysV_AMD64;

    extensions: [sse, sse2, avx, avx2];
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());
    ASSERT_EQ(ast->m_extensions.size(), 4u);
    EXPECT_EQ(ast->m_extensions[0].m_name.m_node, "sse");
    EXPECT_EQ(ast->m_extensions[1].m_name.m_node, "sse2");
    EXPECT_EQ(ast->m_extensions[2].m_name.m_node, "avx");
    EXPECT_EQ(ast->m_extensions[3].m_name.m_node, "avx2");
}

/**
 * Verifies inline register banks and special registers parse correctly inside .tdesc.
 */
TEST_F(TargetDescDefLangTest, ParsesInlineRegisterBanksAndSpecials)
{
    std::string source = R"(
target X86_64 {
    pointer_size: 8;
    stack_slot:   8;
    object_formats: [ELF, COFF];
    default_calling_conv: SysV_AMD64;

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
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());
    EXPECT_EQ(ast->m_name.m_node, "X86_64");
    ASSERT_EQ(ast->m_registerBanks.size(), 1u);

    const auto &bank = ast->m_registerBanks[0];
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

/**
 * Verifies multiple register banks and optional sub_register blocks.
 */
TEST_F(TargetDescDefLangTest, ParsesMultipleInlineRegisterBanks)
{
    std::string source = R"(
target X86_64 {
    pointer_size: 8;
    stack_slot:   8;

    register_bank GPR {
        classes { GPR64: 64 }
        registers {
            rax enc 0 names { rax: GPR64 }
        }
    }

    register_bank FPR {
        classes { FPR32: 32, FPR64: 64, VR128: 128 }
        sub_register { VR128 <: FPR64, FPR64 <: FPR32 }
        registers {
            xmm0 enc 0 names { xmm0: VR128, xmm0: FPR64, xmm0: FPR32 }
        }
    }
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());
    ASSERT_EQ(ast->m_registerBanks.size(), 2u);
    EXPECT_EQ(ast->m_registerBanks[0].m_name.m_node, "GPR");
    EXPECT_EQ(ast->m_registerBanks[1].m_name.m_node, "FPR");
    ASSERT_EQ(ast->m_registerBanks[1].m_subRegisterEdges.size(), 2u);
}

/**
 * Verifies grouped registers { ... } block syntax in .tdesc.
 */
TEST_F(TargetDescDefLangTest, ParsesGroupedRegistersBlock)
{
    std::string source = R"(
target X86_64 {
    pointer_size: 8;
    stack_slot:   8;

    registers {
        register_bank GPR {
            classes { GPR64: 64 }
            registers {
                rax enc 0 names { rax: GPR64 }
            }
        }
        special {
            rip: 16
        }
    }
}
)";

    auto ast = parse(source);
    ASSERT_TRUE(ast.has_value());
    ASSERT_EQ(ast->m_registerBanks.size(), 1u);
    EXPECT_EQ(ast->m_registerBanks[0].m_name.m_node, "GPR");
    ASSERT_EQ(ast->m_specialRegs.size(), 1u);
    EXPECT_EQ(ast->m_specialRegs[0].m_name.m_node, "rip");
}


