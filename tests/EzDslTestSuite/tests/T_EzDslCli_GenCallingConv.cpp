#include "EzDslTestSuite.h"
#include "Ast/CallingConvDefLangAst.h"
#include "Ast/TargetDefLangAst.h"
#include "CodeGenerators/CppCallingConvGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/CallingConvDefLang.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/CallingConvPass.h"
#include "SemaPasses/RegisterBankPass.h"
#include "SemaPasses/TargetDefPass.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

class CallingConvGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_tempDir =
                std::filesystem::temp_directory_path() / ("ezdsl_cc_test_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(m_tempDir);
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(m_tempDir, ec);
        DslTestSuiteAsGtest::TearDown();
    }

    std::string readFile(const std::filesystem::path &filePath) const
    {
        std::ifstream file(filePath);
        if (!file.is_open()) return {};
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::filesystem::path m_tempDir;
};

TEST_F(CallingConvGeneratorTest, GeneratesCallingConventionDescriptors)
{
    std::string targetDsl = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(GPR64,
            rax(, 64, 0),
            rcx(, 64, 0),
            rdx(, 64, 0),
            rsi(, 64, 0),
            rdi(, 64, 0),
            rsp(, 64, 0),
            rbp(, 64, 0)
        );
    };
};
)dsl";

    std::string ccdfDsl = R"dsl(
calling_conv SystemV_AMD64 {
    STACK_ALIGN(16);
    STACK_DIRECTION(DOWN);
    STACK_CLEANUP(CALLER);
    SHADOW_SPACE(0);

    STACK_POINTER(GPR64:rsp);
    FRAME_POINTER(GPR64:rbp);

    CALLEE_SAVED(GPR64:rbp);
    CALLER_SAVED(GPR64:rax, GPR64:rcx, GPR64:rdx, GPR64:rsi, GPR64:rdi);
};
)dsl";

    SymbolTable symTable(getAllocator());

    ParseContext targetCtx = createParseContextFromBuff("target.tdf", targetDsl);
    auto targetAst = targetCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(targetAst.has_value());
    ASSERT_TRUE(TargetDefPass::run(getDiagCollector(), &symTable, &*targetAst));
    ASSERT_TRUE(RegisterBankPass::run(getDiagCollector(), &symTable, &*targetAst));

    ParseContext ccdfCtx = createParseContextFromBuff("abi.ccdf", ccdfDsl);
    auto ccdfAst = ccdfCtx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile, DSL::Ast::CallingConvDef::CallingConvDefFile>();
    ASSERT_TRUE(ccdfAst.has_value());
    ASSERT_TRUE(CallingConvPass::run(getDiagCollector(), &symTable, &*ccdfAst));

    ASSERT_TRUE(CodeGenerators::GenerateTargetCallingConventions(getDiagCollector(),
                                                                &symTable,
                                                                m_tempDir,
                                                                "x86_64",
                                                                CodeGenerators::CallingConvGenWorkingMode::Full));

    auto headerPath = m_tempDir / "x86_64CallingConventions.h";
    auto sourcePath = m_tempDir / "x86_64CallingConventions.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFile(headerPath);
    EXPECT_NE(headerContent.find("class SystemV_AMD64_CallingConvDesc : public CallingConvDesc"), std::string::npos);
    EXPECT_NE(headerContent.find("CallingConvDesc *Create_SystemV_AMD64(std::pmr::memory_resource *alloc);"),
              std::string::npos);
}
