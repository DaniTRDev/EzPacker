#include "EzDslTestSuite.h"
#include "Ast/TypeDefLangAst.h"
#include "CodeGenerators/CppTargetTypeLayoutGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TypeDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/TypePass.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

class TargetTypeLayoutGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_tempDir =
                std::filesystem::temp_directory_path() / ("ezdsl_layout_test_" + std::to_string(std::random_device{}()));
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

TEST_F(TargetTypeLayoutGeneratorTest, GeneratesTypeLayoutImplementation)
{
    std::string typeDsl = R"dsl(
integer i8(8);
integer i32(32);
integer i64(64);
)dsl";

    ParseContext typeCtx = createParseContextFromBuff("types.tyf", typeDsl);
    auto typeAst = typeCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(typeAst.has_value());

    SymbolTable symTable(getAllocator());
    TypePass typePass;
    ASSERT_TRUE(typePass.run(getDiagCollector(), &symTable, &*typeAst));

    ASSERT_TRUE(CodeGenerators::GenerateTargetTypeLayout(getDiagCollector(),
                                                        &symTable,
                                                        m_tempDir,
                                                        "x86_64",
                                                        CodeGenerators::TargetTypeLayoutGenWorkingMode::Full));

    auto headerPath = m_tempDir / "x86_64TypeLayout.h";
    auto sourcePath = m_tempDir / "x86_64TypeLayout.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFile(headerPath);
    EXPECT_NE(headerContent.find("class x86_64TargetTypeLayout : public IMirTargetTypeLayout"), std::string::npos);
    EXPECT_NE(headerContent.find("size_t getPointerSizeInBytes() const override;"), std::string::npos);

    std::string sourceContent = readFile(sourcePath);
    EXPECT_NE(sourceContent.find("x86_64TargetTypeLayout::getPointerSizeInBytes()"), std::string::npos);
}
