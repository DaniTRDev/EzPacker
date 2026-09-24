#include "EzDslCodeGeneratorsTestSuite.h"
#include "Ast/InstructionSelectDefLangAst.h"
#include "CodeGenerators/CppInstructionSelectorGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/InstructionSelectDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/InstructionSelectPass.h"

#include <filesystem>

using namespace CodeGenerators;

/**
 * Fixture for generating a C++ instruction selector from instruction-select (.isf) patterns.
 */
class CppInstructionSelectorGeneratorTest : public EzDslCodeGeneratorsTestSuiteAsGtest
{
  protected:
    std::optional<ParseContext> m_ctx;
    std::optional<DSL::Ast::InstructionSelectDef::InstructionSelectFile> m_ast;

    // Parses ISF source and runs the instruction-select semantic pass, returning success.
    bool parseAndRunPass(const std::string &source)
    {
        m_ctx.emplace(createParseContextFromBuff("isel_test", source));
        m_ast = m_ctx->parse<DSL::Parser::InstructionSelectDef::InstructionSelectFile,
                             DSL::Ast::InstructionSelectDef::InstructionSelectFile>();
        if (!m_ast.has_value())
        {
            return false;
        }

        return InstructionSelectPass::run(getDiagCollector(), getSymbolTable(), &m_ast.value());
    }

    void TearDown() override
    {
        m_ast.reset();
        m_ctx.reset();
        EzDslCodeGeneratorsTestSuiteAsGtest::TearDown();
    }
};

// Generates an instruction selector and checks the emitted dispatch, selection helpers, and predicates.
TEST_F(CppInstructionSelectorGeneratorTest, TestInstructionSelectorGeneration)
{
    std::string isfSource = R"(
        target AMD64;

        addrmode AddrModeRegImm(GPR64:base, simm32:disp = 0) {
            variant BaseDisp {
                match {
                    ADD ptr:$base, imm(i32):$disp;
                };
                when {
                    isSimm32($disp);
                };
            };
            variant BaseOnly {
                match {
                    ptr:$base;
                };
            };
        };

        pattern Select_ADD32rr [cost = 1] {
            match {
                ADD i32:$dst, i32:$src1, i32:$src2;
            };
            select {
                ADD32rr GPR32:$dst, GPR32:$src1, GPR32:$src2;
            };
        };

        pattern Select_ADD32ri [cost = 1] {
            match {
                ADD i32:$dst, i32:$src1, imm(i32):$imm;
            };
            when {
                isSimm32($imm);
            };
            select {
                ADD32ri GPR32:$dst, GPR32:$src1, $imm;
            };
        };

        pattern Select_ADD32rm [cost = 2] {
            match {
                ADD i32:$dst, i32:$src1, (LOAD i32:$tmp, AddrModeRegImm($base, $disp));
            };
            when {
                hasOneUse($tmp);
                noInterveningStore($tmp);
            };
            select {
                ADD32rm GPR32:$dst, GPR32:$src1, [$base, $disp];
            };
        };
    )";

    ASSERT_TRUE(parseAndRunPass(isfSource));

    CppInstructionSelectorGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    auto headerPath = m_testTempDir / "AMD64InstructionSelector.h";
    auto sourcePath = m_testTempDir / "AMD64InstructionSelector.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFileContent(headerPath);
    std::string sourceContent = readFileContent(sourcePath);

    // Verify Header
    EXPECT_NE(headerContent.find("EZTARGETS_AMD64_INSTRUCTION_SELECTOR_H"), std::string::npos);
    EXPECT_NE(headerContent.find("class AMD64InstructionSelector : public MirInstructionSelector"), std::string::npos);
    EXPECT_NE(headerContent.find("bool select(MirBuilderContext *ctx, MirInstruction *inst) override;"),
              std::string::npos);
    EXPECT_NE(headerContent.find("bool selectADD(MirBuilderContext *ctx, MirInstruction *inst);"), std::string::npos);

    // Verify Source
    EXPECT_NE(sourceContent.find("AMD64InstructionSelector::select(MirBuilderContext *ctx, MirInstruction *inst)"),
              std::string::npos);
    EXPECT_NE(sourceContent.find("case MirInstructionOpCode::ADD:"), std::string::npos);
    EXPECT_NE(sourceContent.find(
                      "bool AMD64InstructionSelector::selectADD(MirBuilderContext *ctx, MirInstruction *inst)"),
              std::string::npos);
    EXPECT_NE(sourceContent.find("findClass(\"GPR32\")"), std::string::npos);
    EXPECT_NE(sourceContent.find("AMD64TargetInst::getTargetDesc(AMD64TargetInst::ADD32rm)"), std::string::npos);
    EXPECT_NE(sourceContent.find("AMD64TargetInst::getTargetDesc(AMD64TargetInst::ADD32rr)"), std::string::npos);
    EXPECT_NE(sourceContent.find("AMD64TargetInst::getTargetDesc(AMD64TargetInst::ADD32ri)"), std::string::npos);
    EXPECT_NE(sourceContent.find("hasOneUse(vregOp)"), std::string::npos);
    EXPECT_NE(sourceContent.find("noInterveningStore(defInst, inst)"), std::string::npos);
    EXPECT_NE(sourceContent.find("isSimm32"), std::string::npos);
}

// Verifies when { hasExtension("avx"); } generates a m_targetDesc->hasExtension check.
TEST_F(CppInstructionSelectorGeneratorTest, TestHasExtensionGuardGeneration)
{
    std::string isfSource = R"(
        target AMD64;

        pattern Select_ADD32rr_AVX [cost = 1] {
            match {
                ADD i32:$dst, i32:$src1, i32:$src2;
            };
            when {
                hasExtension("avx");
            };
            select {
                VADD32rr GPR32:$dst, GPR32:$src1, GPR32:$src2;
            };
        };
    )";

    ASSERT_TRUE(parseAndRunPass(isfSource));

    CppInstructionSelectorGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    auto sourcePath = m_testTempDir / "AMD64InstructionSelector.cpp";
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string sourceContent = readFileContent(sourcePath);
    EXPECT_NE(sourceContent.find("if (m_targetDesc && m_targetDesc->hasExtension(\"avx\"))"), std::string::npos);
}

