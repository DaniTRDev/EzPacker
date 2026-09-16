#include "EzDslCodeGeneratorsTestSuite.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "CodeGenerators/CppLegalizerGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeActionPass.h"

#include <filesystem>
#include <fstream>

using namespace CodeGenerators;

class CppLegalizerGeneratorTest : public EzDslCodeGeneratorsTestSuiteAsGtest
{
  protected:
    void SetUp() override
    {
        EzDslCodeGeneratorsTestSuiteAsGtest::SetUp();
        declareStandardTypes();
        declareStandardIrInstructions();
    }

    void declareStandardTypes()
    {
        auto declareType = [&](std::string_view name, DSL::Ast::TypeDef::TypeKind kind, uint32_t bitWidth, uint8_t compactId) {
            Symbols::TypeSymbol symData{
                .m_name = name,
                .m_kind = kind,
                .m_bitWidth = bitWidth,
                .m_alignment = bitWidth,
                .m_compactId = compactId
            };
            getSymbolTable()->declareSym(nullptr, SymbolType::Type, std::move(symData), name);
        };

        declareType("i1", DSL::Ast::TypeDef::TypeKind::Integer, 1, 4);
        declareType("i8", DSL::Ast::TypeDef::TypeKind::Integer, 8, 5);
        declareType("i16", DSL::Ast::TypeDef::TypeKind::Integer, 16, 6);
        declareType("i32", DSL::Ast::TypeDef::TypeKind::Integer, 32, 7);
        declareType("i64", DSL::Ast::TypeDef::TypeKind::Integer, 64, 8);
        declareType("i128", DSL::Ast::TypeDef::TypeKind::Integer, 128, 9);
        declareType("f32", DSL::Ast::TypeDef::TypeKind::FloatingPoint, 32, 11);
        declareType("f64", DSL::Ast::TypeDef::TypeKind::FloatingPoint, 64, 12);
        declareType("ptr", DSL::Ast::TypeDef::TypeKind::Pointer, 64, 3);
    }

    void declareStandardIrInstructions()
    {
        auto declareInst = [&](std::string_view name) {
            Symbols::IrInstructionSymbol symData{
                .m_name = name,
                .m_category = DSL::Ast::IrInstDef::IrInstCategory::Arithmetic,
                .m_tier = DSL::Ast::IrInstDef::IrInstTier::HighLevel,
                .m_flags = DSL::Ast::IrInstDef::IrInstFlag::None,
                .m_operands = std::pmr::vector<Symbols::IrOperandSymbol>{ getSymbolTable()->getAllocator() }
            };
            getSymbolTable()->declareSym(nullptr, SymbolType::IrInstruction, std::move(symData), name);
        };

        declareInst("ADD");
        declareInst("SUB");
        declareInst("AND");
        declareInst("OR");
        declareInst("XOR");
        declareInst("SDIV");
        declareInst("SEXT");
        declareInst("STORE");
        declareInst("CALL");
        declareInst("RET");
        declareInst("ALLOC");
    }

    std::optional<DSL::Ast::LegalizeActionDef::LegalizeActionFile> parseLad(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.lad", m_testId++), source);
        return ctx.parse<DSL::Parser::LegalizeActionDef::LegalizeActionFile,
                         DSL::Ast::LegalizeActionDef::LegalizeActionFile>();
    }

  private:
    size_t m_testId{ 0 };
};

TEST_F(CppLegalizerGeneratorTest, TestFullTargetGeneration)
{
    std::string source = R"dsl(
target AMD64;

type_set GPR_SCALARS = (i8, i16, i32, i64);

action ADD {
    CLAMP_SCALAR(i32, i64);
};

action SDIV {
    LEGAL(i32, i64);
    WIDENS(i8, i16) >> i32;
    LIBCALL(i128)   >> "__divti3";
};

action STORE {
    LEGAL(GPR_SCALARS:0, ptr:1);
    WIDENS(i1:0) >> i8;
};

action CALL {
    LOWER >> AMD64CallLowering;
};

action RET {
    LOWER >> AMD64ReturnLowering;
};
)dsl";

    auto ast = parseLad(source);
    ASSERT_TRUE(ast.has_value());

    bool semaOk = LegalizeActionPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    ASSERT_TRUE(semaOk);

    auto tempDir = std::filesystem::temp_directory_path() / "ezdsl_legalizer_test";
    std::filesystem::create_directories(tempDir);

    CppLegalizerGenerator generator(getDiagCollector(), getSymbolTable(), tempDir, "AMD64");
    bool genOk = generator.run();
    EXPECT_TRUE(genOk);

    auto headerPath = tempDir / "AMD64LegalizerActionTable.h";
    auto sourcePath = tempDir / "AMD64LegalizerActionTable.cpp";

    EXPECT_TRUE(std::filesystem::exists(headerPath));
    EXPECT_TRUE(std::filesystem::exists(sourcePath));

    // Inspect header contents
    std::ifstream hFile(headerPath);
    std::string hContent((std::istreambuf_iterator<char>(hFile)), std::istreambuf_iterator<char>());
    EXPECT_NE(hContent.find("class AMD64LegalizerInfo : public LegalizerInfo"), std::string::npos);
    EXPECT_NE(hContent.find("LegalityResponse query(const LegalityQuery &q) const override;"), std::string::npos);
    EXPECT_NE(hContent.find("LegalizationResult executeCustom(LegalizeCtx &ctx, uint16_t handlerId) override;"), std::string::npos);

    // Inspect source contents
    std::ifstream sFile(sourcePath);
    std::string sContent((std::istreambuf_iterator<char>(sFile)), std::istreambuf_iterator<char>());

    // Verify libcall pool
    EXPECT_NE(sContent.find("\"__divti3\""), std::string::npos);

    // Verify lowering forward declarations
    EXPECT_NE(sContent.find("extern LegalizationResult AMD64CallLowering(LegalizeCtx &ctx);"), std::string::npos);
    EXPECT_NE(sContent.find("extern LegalizationResult AMD64ReturnLowering(LegalizeCtx &ctx);"), std::string::npos);

    // Verify Tier 2 heterogeneous matcher for STORE
    EXPECT_NE(sContent.find("static LegalityResponse match_STORE(const LegalityQuery &q)"), std::string::npos);

    // Verify Tier 3 wildcard table for CALL and RET
    EXPECT_NE(sContent.find("g_AMD64_WildcardActions"), std::string::npos);

    // Verify Tier 1 dense matrix
    EXPECT_NE(sContent.find("g_AMD64_PrimaryMatrix"), std::string::npos);

    // Verify executeCustom switch
    EXPECT_NE(sContent.find("case 0: return AMD64CallLowering(ctx);"), std::string::npos);
    EXPECT_NE(sContent.find("case 1: return AMD64ReturnLowering(ctx);"), std::string::npos);

    hFile.close();
    sFile.close();

    // Clean up
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
}
