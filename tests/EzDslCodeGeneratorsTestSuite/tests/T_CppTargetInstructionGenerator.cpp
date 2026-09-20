#include "EzDslCodeGeneratorsTestSuite.h"
#include "Ast/TargetInstDefLangAst.h"
#include "CodeGenerators/CppTargetInstructionGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetInstDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/TargetInstPass.h"

#include <filesystem>

using namespace CodeGenerators;

/**
 * Fixture for generating target instruction tables from target-instruction (.idf) sources.
 */
class CppTargetInstructionGeneratorTest : public EzDslCodeGeneratorsTestSuiteAsGtest
{
  protected:
    // Parses target-instruction source and runs the target-instruction semantic pass.
    bool parseAndRunPass(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("target_test", source);
        auto ast = ctx.parse<DSL::Parser::TargetInstDef::TargetInstFile, DSL::Ast::TargetInstDef::TargetInstFile>();
        if (!ast.has_value())
        {
            return false;
        }

        return TargetInstPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    }
};

// Generates a target instruction table and verifies the opcode enum, descriptor array, flags, and initializer.
TEST_F(CppTargetInstructionGeneratorTest, TestTargetInstructionTableGeneration)
{
    std::string idfSource = R"(
        target AMD64;

        target_inst ADD32rr(GPR32:dst OUT, GPR32:src1 IN, GPR32:src2 IN) {
            MNEMONIC("addl");
            FLAGS(IsCommutative);
            IMPLICIT_DEFS(EFLAGS);
        };

        target_inst ADD32rm(GPR32:dst OUT, GPR32:src IN, Mem32:addr IN) {
            MNEMONIC("addl");
            FLAGS(ReadsMemory);
            IMPLICIT_DEFS(EFLAGS);
        };
    )";

    ASSERT_TRUE(parseAndRunPass(idfSource));

    CppTargetInstructionGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    auto headerPath = m_testTempDir / "AMD64TargetInstructionTable.h";
    auto sourcePath = m_testTempDir / "AMD64TargetInstructionTable.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFileContent(headerPath);
    std::string sourceContent = readFileContent(sourcePath);

    // Verify Header
    EXPECT_NE(headerContent.find("EZTARGETS_AMD64_TARGET_INSTRUCTION_TABLE_H"), std::string::npos);
    EXPECT_NE(headerContent.find("namespace EzTargets::AMD64TargetInst"), std::string::npos);
    EXPECT_NE(headerContent.find("enum OpCode : size_t"), std::string::npos);
    EXPECT_NE(headerContent.find("ADD32rr = 1"), std::string::npos);
    EXPECT_NE(headerContent.find("ADD32rm = 2"), std::string::npos);
    EXPECT_NE(headerContent.find("OPCODE_COUNT = 3"), std::string::npos);
    EXPECT_NE(headerContent.find("const MirTargetInstructionDesc *getTargetDesc(OpCode op);"), std::string::npos);
    EXPECT_NE(headerContent.find("void initializeTargetInstructionTable(::TargetDesc *target);"), std::string::npos);

    // Verify Source
    EXPECT_NE(sourceContent.find("namespace EzTargets::AMD64TargetInst"), std::string::npos);
    EXPECT_NE(sourceContent.find("static MirTargetInstructionDesc s_descs[] ="), std::string::npos);
    EXPECT_NE(sourceContent.find("\"ADD32rr\""), std::string::npos);
    EXPECT_NE(sourceContent.find("MirInstructionFlags::IsCommutative"), std::string::npos);
    EXPECT_NE(sourceContent.find("MirInstructionFlags::ReadsMemory"), std::string::npos);
    EXPECT_NE(sourceContent.find("const MirTargetInstructionDesc *getTargetDesc(OpCode op)"), std::string::npos);
    EXPECT_NE(sourceContent.find("void initializeTargetInstructionTable(::TargetDesc *target)"), std::string::npos);
    EXPECT_NE(sourceContent.find("setOperandClass"), std::string::npos);
}

// WEI-02: an alias target name containing a separator must still yield compilable identifiers.
TEST_F(CppTargetInstructionGeneratorTest, TestDashedTargetNameIsSanitized)
{
    std::string idfSource = R"(
        target AMD64;

        target_inst ADD32rr(GPR32:dst OUT, GPR32:src1 IN, GPR32:src2 IN) {
            MNEMONIC("addl");
        };
    )";

    ASSERT_TRUE(parseAndRunPass(idfSource));

    CppTargetInstructionGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "x86-64");
    ASSERT_TRUE(generator.run());

    // 'x86-64' must be rewritten to 'x86_64' in the file names and emitted identifiers.
    auto headerPath = m_testTempDir / "x86_64TargetInstructionTable.h";
    auto sourcePath = m_testTempDir / "x86_64TargetInstructionTable.cpp";
    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFileContent(headerPath);
    EXPECT_NE(headerContent.find("EZTARGETS_X86_64_TARGET_INSTRUCTION_TABLE_H"), std::string::npos);
    EXPECT_NE(headerContent.find("namespace EzTargets::x86_64TargetInst"), std::string::npos);
    EXPECT_EQ(headerContent.find("x86-64"), std::string::npos);
}
