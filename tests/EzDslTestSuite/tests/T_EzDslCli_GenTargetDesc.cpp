#include "EzDslTestSuite.h"
#include "Ast/TargetDefLangAst.h"
#include "CodeGenerators/CppTargetDescGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/RegisterBankPass.h"
#include "SemaPasses/TargetDefPass.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

class TargetDescGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_tempDir =
                std::filesystem::temp_directory_path() / ("ezdsl_desc_test_" + std::to_string(std::random_device{}()));
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

TEST_F(TargetDescGeneratorTest, GeneratesTargetDescGlue)
{
    std::string targetDsl = R"dsl(
target AMD64 {
    bank GPR {
        CLASS(GPR64, rax(, 64, 0));
    };
};
)dsl";

    ParseContext targetCtx = createParseContextFromBuff("target.tdf", targetDsl);
    auto targetAst = targetCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(targetAst.has_value());

    SymbolTable symTable(getAllocator());
    ASSERT_TRUE(TargetDefPass::run(getDiagCollector(), &symTable, &*targetAst));
    ASSERT_TRUE(RegisterBankPass::run(getDiagCollector(), &symTable, &*targetAst));

    ASSERT_TRUE(CodeGenerators::GenerateTargetDescriptor(getDiagCollector(),
                                                        &symTable,
                                                        m_tempDir,
                                                        "AMD64",
                                                        CodeGenerators::TargetDescGenWorkingMode::Full));

    auto headerPath = m_tempDir / "AMD64TargetDesc.h";
    auto sourcePath = m_tempDir / "AMD64TargetDesc.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFile(headerPath);
    EXPECT_NE(headerContent.find("class AMD64TargetDesc : public TargetDesc"), std::string::npos);
    EXPECT_NE(headerContent.find("std::unique_ptr<TargetDesc> CreateAMD64TargetDesc(std::pmr::memory_resource *alloc);"),
              std::string::npos);

    std::string sourceContent = readFile(sourcePath);
    EXPECT_NE(sourceContent.find("AMD64TargetDesc::getName()"), std::string::npos);
    EXPECT_NE(sourceContent.find("CreateAMD64TargetDesc"), std::string::npos);
}
