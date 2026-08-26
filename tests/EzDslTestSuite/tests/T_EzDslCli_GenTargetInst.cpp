#include "EzDslTestSuite.h"
#include "Ast/InstructionDefLangAst.h"
#include "Ast/TargetDefLangAst.h"
#include "CodeGenerators/CppTargetInstGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/InstructionDefLang.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/RegisterBankPass.h"
#include "SemaPasses/TargetDefPass.h"
#include "SemaPasses/TargetInstPass.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

/**
 * Test fixture for target instruction and binary encoder code generator (CppTargetInstGenerator).
 */
class TargetInstGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_tempDir =
                std::filesystem::temp_directory_path() / ("ezdsl_inst_test_" + std::to_string(std::random_device{}()));
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

TEST_F(TargetInstGeneratorTest, GeneratesInstructionDefsAndEncoders)
{
    std::string targetDsl = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(GPR64, rax(, 64, 0), rcx(, 64, 0));
    };
};
)dsl";

    std::string idfDsl = R"dsl(
format R_TYPE(32) {
    opcode[0:7];
    rd[8:15];
    rs1[16:23];
    rs2[24:31];
};

inst ADD(GPR64:rd OUT, GPR64:rs1 IN, GPR64:rs2 IN) format R_TYPE {
    FORMAT(opcode = 0x01, rd = rd, rs1 = rs1, rs2 = rs2);
    ASM("add $rd, $rs1, $rs2");
}
)dsl";

    SymbolTable symTable(getAllocator());

    // 1. Parse & Run TargetDefPass + RegisterBankPass
    ParseContext targetCtx = createParseContextFromBuff("target.tdf", targetDsl);
    auto targetAst = targetCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(targetAst.has_value());
    ASSERT_TRUE(TargetDefPass::run(getDiagCollector(), &symTable, &*targetAst));
    ASSERT_TRUE(RegisterBankPass::run(getDiagCollector(), &symTable, &*targetAst));

    // 2. Parse & Run InstructionDefPass
    ParseContext idfCtx = createParseContextFromBuff("insts.idf", idfDsl);
    auto idfAst = idfCtx.parse<DSL::Parser::InstDef::InstDefFile, DSL::Ast::InstDef::InstDefFile>();
    ASSERT_TRUE(idfAst.has_value());
    ASSERT_TRUE(InstructionDefPass::run(getDiagCollector(), &symTable, &*idfAst));

    // 3. Generate
    ASSERT_TRUE(CodeGenerators::GenerateTargetInstructionDefs(getDiagCollector(),
                                                             &symTable,
                                                             m_tempDir,
                                                             "x86_64",
                                                             CodeGenerators::TargetInstGenWorkingMode::Full));

    auto headerPath = m_tempDir / "x86_64InstructionDefs.h";
    auto sourcePath = m_tempDir / "x86_64InstructionDefs.cpp";
    auto encoderPath = m_tempDir / "x86_64BinaryEncoder.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));
    ASSERT_TRUE(std::filesystem::exists(encoderPath));

    std::string headerContent = readFile(headerPath);
    EXPECT_NE(headerContent.find("namespace x86_64"), std::string::npos);
    EXPECT_NE(headerContent.find("enum class TargetOpCode : uint16_t"), std::string::npos);
    EXPECT_NE(headerContent.find("ADD,"), std::string::npos);

    std::string sourceContent = readFile(sourcePath);
    EXPECT_NE(sourceContent.find("GetTargetInstructionDesc"), std::string::npos);
    EXPECT_NE(sourceContent.find("PrintTargetInstruction"), std::string::npos);

    std::string encoderContent = readFile(encoderPath);
    EXPECT_NE(encoderContent.find("Encode_ADD"), std::string::npos);
    EXPECT_NE(encoderContent.find("EncodeTargetInstruction"), std::string::npos);
}
