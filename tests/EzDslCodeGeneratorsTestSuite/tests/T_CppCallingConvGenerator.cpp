#include "EzDslCodeGeneratorsTestSuite.h"
#include "Ast/CallingConvDefLangAst.h"
#include "CodeGenerators/CppCallingConvGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/CallingConvDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/CallingConvPass.h"

#include <filesystem>
#include <fstream>

using namespace CodeGenerators;

/**
 * Fixture for generating C++ calling-convention descriptor classes from .ezcc sources.
 */
class CppCallingConvGeneratorTest : public EzDslCodeGeneratorsTestSuiteAsGtest
{
  protected:
    // Parses calling-convention source into an AST using a unique .ezcc source name.
    std::optional<DSL::Ast::CallingConvDef::CallingConventionDefFile> parseCallingConv(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.ezcc", m_testId++), source);
        return ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile,
                         DSL::Ast::CallingConvDef::CallingConventionDefFile>();
    }

  private:
    size_t m_testId{ 0 };
};

// Generates a SysV_AMD64 descriptor and checks the emitted header/source for the expected accessors.
TEST_F(CppCallingConvGeneratorTest, TestFullCallingConvGeneration)
{
    std::string source = R"(
    calling_convention SysV_AMD64 {
        stack {
            align: 16,
            growth: down,
            cleanup: caller,
            shadow_space: 0,
            red_zone: 128,
            sp: rsp,
            fp: rbp
        }

        preserve callee: [rbx, rsp, rbp, r12, r13, r14, r15]
        preserve caller: [rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11]

        classify {
            types [i8, i16, i32, i64, ptr] => integer
            types [f32, f64] => sse
            aggregate {
                when non_trivial || unaligned || size > 64 => memory,
                slice: 8 bytes,
                precedence: [memory, integer, sse],
                policy: all_or_nothing
            }
        }

        arguments {
            pass integer => seq([rdi, rsi, rdx, rcx, r8, r9]), fallback: stack(8)
            pass sse => seq([xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7]), fallback: stack(8)
            pass memory => stack(8)
        }

        returns {
            sret {
                ptr: rdi,
                consumes_slot: true,
                returns: rax
            }
            pass integer => seq([rax, rdx])
            pass sse => seq([xmm0, xmm1])
        }

        varargs {
            vector_count_reg: al,
            duplicate_floats_to_gpr: false,
            stack_align: 8
        }
    }
    )";

    auto ast = parseCallingConv(source);
    ASSERT_TRUE(ast.has_value());

    bool semaOk = CallingConvPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    ASSERT_TRUE(semaOk);

    std::filesystem::path outDir = std::filesystem::temp_directory_path() / "ezdsl_cc_test";
    std::filesystem::create_directories(outDir);

    CppCallingConvGenerator gen(getDiagCollector(), getSymbolTable(), outDir, "AMD64");
    bool genOk = gen.run();
    EXPECT_TRUE(genOk);

    auto headerPath = outDir / "AMD64CallingConvDesc.h";
    auto sourcePath = outDir / "AMD64CallingConvDesc.cpp";

    EXPECT_TRUE(std::filesystem::exists(headerPath));
    EXPECT_TRUE(std::filesystem::exists(sourcePath));

    std::ifstream headerStream(headerPath);
    std::string headerContent((std::istreambuf_iterator<char>(headerStream)), std::istreambuf_iterator<char>());

    EXPECT_NE(headerContent.find("class SysV_AMD64CallingConvDesc : public CallingConvDesc"), std::string::npos);
    EXPECT_NE(headerContent.find("ArgumentLocationDesc getArgLoc"), std::string::npos);
    EXPECT_NE(headerContent.find("ArgumentLocationDesc getReturnLoc"), std::string::npos);
    EXPECT_NE(headerContent.find("size_t getRedZoneSize() const override;"), std::string::npos);
    EXPECT_NE(headerContent.find("bool consumesSretSlot() const override;"), std::string::npos);

    headerStream.close();

    std::ifstream sourceStream(sourcePath);
    std::string sourceContent((std::istreambuf_iterator<char>(sourceStream)), std::istreambuf_iterator<char>());
    sourceStream.close();

    EXPECT_NE(sourceContent.find("SysV_AMD64CallingConvDesc::getStackAlignment"), std::string::npos);
    EXPECT_NE(sourceContent.find("SysV_AMD64CallingConvDesc::getRedZoneSize"), std::string::npos);
    EXPECT_NE(sourceContent.find("return 128;"), std::string::npos);
    EXPECT_NE(sourceContent.find("return 16;"), std::string::npos);
    EXPECT_NE(sourceContent.find("SysV_AMD64CallingConvDesc::getArgLoc"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove_all(outDir, ec);
}

// Generates a Win64 descriptor and verifies shadow-space and sret-slot handling in the output.
TEST_F(CppCallingConvGeneratorTest, TestWin64CallingConvGeneration)
{
    std::string source = R"(
    calling_convention Win64 {
        stack {
            align: 16,
            growth: down,
            cleanup: caller,
            shadow_space: 32,
            sp: rsp,
            fp: rbp
        }

        preserve callee: [rbx, rbp, rdi, rsi, r12, r13, r14, r15]
        preserve caller: [rax, rcx, rdx, r8, r9, r10, r11]

        classify {
            types [i8, i16, i32, i64, ptr] => integer
            types [f32, f64] => sse
        }

        arguments {
            pass integer => seq([rcx, rdx, r8, r9]), fallback: stack(8)
            pass sse => seq([xmm0, xmm1, xmm2, xmm3]), fallback: stack(8)
            pass memory => stack(8)
        }

        returns {
            sret {
                ptr: rcx,
                consumes_slot: true,
                returns: rax
            }
            pass integer => seq([rax])
            pass sse => seq([xmm0])
        }
    }
    )";

    auto ast = parseCallingConv(source);
    ASSERT_TRUE(ast.has_value());

    bool semaOk = CallingConvPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    ASSERT_TRUE(semaOk);

    std::filesystem::path outDir = std::filesystem::temp_directory_path() / "ezdsl_cc_test_win64";
    std::filesystem::create_directories(outDir);

    CppCallingConvGenerator gen(getDiagCollector(), getSymbolTable(), outDir, "Win64");
    bool genOk = gen.run();
    EXPECT_TRUE(genOk);

    auto headerPath = outDir / "Win64CallingConvDesc.h";
    auto sourcePath = outDir / "Win64CallingConvDesc.cpp";

    EXPECT_TRUE(std::filesystem::exists(headerPath));
    EXPECT_TRUE(std::filesystem::exists(sourcePath));

    std::ifstream headerStream(headerPath);
    std::string headerContent((std::istreambuf_iterator<char>(headerStream)), std::istreambuf_iterator<char>());
    headerStream.close();

    EXPECT_NE(headerContent.find("class Win64CallingConvDesc : public CallingConvDesc"), std::string::npos);
    EXPECT_NE(headerContent.find("size_t getShadowSpaceSize() const override;"), std::string::npos);

    std::ifstream sourceStream(sourcePath);
    std::string sourceContent((std::istreambuf_iterator<char>(sourceStream)), std::istreambuf_iterator<char>());
    sourceStream.close();

    EXPECT_NE(sourceContent.find("Win64CallingConvDesc::getShadowSpaceSize"), std::string::npos);
    EXPECT_NE(sourceContent.find("return 32;"), std::string::npos);
    EXPECT_NE(sourceContent.find("Win64CallingConvDesc::consumesSretSlot"), std::string::npos);
    EXPECT_NE(sourceContent.find("return true;"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove_all(outDir, ec);
}

// Generates an AAPCS64 descriptor and verifies link-register and sret handling in the output.
TEST_F(CppCallingConvGeneratorTest, TestAArch64CallingConvGeneration)
{
    std::string source = R"(
    calling_convention AAPCS64 {
        stack {
            align: 16,
            growth: down,
            cleanup: caller,
            shadow_space: 0,
            sp: sp,
            fp: x29,
            lr: x30
        }

        preserve callee: [x19, x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30, d8, d9, d10, d11, d12, d13, d14, d15]
        preserve caller: [x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18]

        classify {
            types [i8, i16, i32, i64, ptr] => integer
            types [f32, f64] => sse
        }

        arguments {
            pass integer => seq([x0, x1, x2, x3, x4, x5, x6, x7]), fallback: stack(8)
            pass sse => seq([v0, v1, v2, v3, v4, v5, v6, v7]), fallback: stack(8)
            pass memory => stack(8)
        }

        returns {
            sret {
                ptr: x8,
                consumes_slot: false
            }
            pass integer => seq([x0, x1, x2, x3, x4, x5, x6, x7])
            pass sse => seq([v0, v1, v2, v3])
        }
    }
    )";

    auto ast = parseCallingConv(source);
    ASSERT_TRUE(ast.has_value());

    bool semaOk = CallingConvPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    ASSERT_TRUE(semaOk);

    std::filesystem::path outDir = std::filesystem::temp_directory_path() / "ezdsl_cc_test_aarch64";
    std::filesystem::create_directories(outDir);

    CppCallingConvGenerator gen(getDiagCollector(), getSymbolTable(), outDir, "AArch64");
    bool genOk = gen.run();
    EXPECT_TRUE(genOk);

    auto headerPath = outDir / "AArch64CallingConvDesc.h";
    auto sourcePath = outDir / "AArch64CallingConvDesc.cpp";

    EXPECT_TRUE(std::filesystem::exists(headerPath));
    EXPECT_TRUE(std::filesystem::exists(sourcePath));

    std::ifstream headerStream(headerPath);
    std::string headerContent((std::istreambuf_iterator<char>(headerStream)), std::istreambuf_iterator<char>());
    headerStream.close();

    EXPECT_NE(headerContent.find("class AAPCS64CallingConvDesc : public CallingConvDesc"), std::string::npos);
    EXPECT_NE(headerContent.find("getLinkRegister() const override;"), std::string::npos);

    std::ifstream sourceStream(sourcePath);
    std::string sourceContent((std::istreambuf_iterator<char>(sourceStream)), std::istreambuf_iterator<char>());
    sourceStream.close();

    EXPECT_NE(sourceContent.find("AAPCS64CallingConvDesc::getLinkRegister"), std::string::npos);
    EXPECT_NE(sourceContent.find("AAPCS64CallingConvDesc::consumesSretSlot"), std::string::npos);
    EXPECT_NE(sourceContent.find("return false;"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove_all(outDir, ec);
}
