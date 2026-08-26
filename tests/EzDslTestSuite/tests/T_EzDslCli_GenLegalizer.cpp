#include "EzDslTestSuite.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "CodeGenerators/CppLegalizerGenerator.h"
#include "CodeGenerators/CppLegalizerRuleGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "Parser/TypeDefLang.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/LegalizeActionPass.h"
#include "SemaPasses/LegalizeRulePass.h"
#include "SemaPasses/TypePass.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

class LegalizerGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_tempDir =
                std::filesystem::temp_directory_path() / ("ezdsl_legal_test_" + std::to_string(std::random_device{}()));
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

TEST_F(LegalizerGeneratorTest, GeneratesLegalizerTableAndRules)
{
    std::string typeDsl = R"dsl(
integer i32(32);
integer i64(64);
)dsl";

    std::string ladDsl = R"dsl(
action ADD {
    LEGAL(i32, i64);
    NARROWS(i64) >> i32;
};
)dsl";

    SymbolTable symTable(getAllocator());

    // Register IR instruction ADD in global scope
    Sema::Symbols::IrInstructionSymbol irSym{ .m_name = "ADD" };
    symTable.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::IrInstruction, irSym, "ADD");

    ParseContext typeCtx = createParseContextFromBuff("types.tyf", typeDsl);
    auto typeAst = typeCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(typeAst.has_value());
    TypePass typePass;
    ASSERT_TRUE(typePass.run(getDiagCollector(), &symTable, &*typeAst));

    // Enter target scope before parsing / validating target legalize actions
    symTable.enterScope("TargetScope");

    ParseContext ladCtx = createParseContextFromBuff("legal.lad", ladDsl);
    auto ladAst = ladCtx.parse<DSL::Parser::LegalizeActionDef::TargetLegalizeDef, DSL::Ast::LegalizeActionDef::TargetLegalizeDef>();
    ASSERT_TRUE(ladAst.has_value());
    ASSERT_TRUE(LegalizeActionPass::run(getDiagCollector(), &symTable, &*ladAst));

    ASSERT_TRUE(CodeGenerators::GenerateTargetLegalizerTable(getDiagCollector(),
                                                            &symTable,
                                                            m_tempDir,
                                                            "x86_64",
                                                            CodeGenerators::LegalizerGenWorkingMode::Full));

    ASSERT_TRUE(CodeGenerators::GenerateTargetLegalizerRules(getDiagCollector(),
                                                            &symTable,
                                                            m_tempDir,
                                                            "x86_64",
                                                            CodeGenerators::LegalizerRuleGenWorkingMode::Full));

    auto tableHeader = m_tempDir / "x86_64LegalizerActionTable.h";
    auto tableSource = m_tempDir / "x86_64LegalizerActionTable.cpp";
    auto rulesHeader = m_tempDir / "x86_64LegalizeRules.h";
    auto rulesSource = m_tempDir / "x86_64LegalizeRules.cpp";

    ASSERT_TRUE(std::filesystem::exists(tableHeader));
    ASSERT_TRUE(std::filesystem::exists(tableSource));
    ASSERT_TRUE(std::filesystem::exists(rulesHeader));
    ASSERT_TRUE(std::filesystem::exists(rulesSource));

    std::string tableContent = readFile(tableSource);
    EXPECT_NE(tableContent.find("GetTargetLegalizeAction"), std::string::npos);
    EXPECT_NE(tableContent.find("IsLegalTargetOperation"), std::string::npos);

    std::string rulesContent = readFile(rulesHeader);
    EXPECT_NE(rulesContent.find("class x86_64LegalizeRules"), std::string::npos);
}
