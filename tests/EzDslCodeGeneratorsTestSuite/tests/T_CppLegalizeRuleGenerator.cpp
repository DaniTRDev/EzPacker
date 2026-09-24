#include "EzDslCodeGeneratorsTestSuite.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "CodeGenerators/CppLegalizeRuleGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeRulePass.h"

#include <filesystem>
#include <fstream>

using namespace CodeGenerators;

/**
 * Fixture for generating C++ legalizer rules from legalize-rule (.lrd) sources.
 */
class CppLegalizeRuleGeneratorTest : public EzDslCodeGeneratorsTestSuiteAsGtest
{
  protected:
    // Declares the standard types and IR instructions needed by the rule generator tests.
    void SetUp() override
    {
        EzDslCodeGeneratorsTestSuiteAsGtest::SetUp();
        declareStandardTypes();
        declareStandardIrInstructions();
    }

    // Registers the primitive integer, float, and pointer types in the symbol table.
    void declareStandardTypes()
    {
        auto declareType =
                [&](std::string_view name, DSL::Ast::TypeDef::TypeKind kind, uint32_t bitWidth, uint8_t compactId)
        {
            Symbols::TypeSymbol symData{ .m_name = name,
                                         .m_kind = kind,
                                         .m_bitWidth = bitWidth,
                                         .m_alignment = bitWidth,
                                         .m_compactId = compactId };
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

    // Registers the IR instructions referenced by the legalize-rule test sources.
    void declareStandardIrInstructions()
    {
        using namespace DSL::Ast::IrInstDef;

        auto declareInst = [&](std::string_view name, std::initializer_list<Symbols::IrOperandSymbol> operands)
        {
            std::pmr::vector<Symbols::IrOperandSymbol> ops(getSymbolTable()->getAllocator());
            for (const auto &op : operands)
                ops.push_back(op);

            Symbols::IrInstructionSymbol symData{ .m_name = name,
                                                  .m_category = IrInstCategory::Arithmetic,
                                                  .m_tier = IrInstTier::HighLevel,
                                                  .m_flags = IrInstFlag::None,
                                                  .m_operands = std::move(ops) };
            getSymbolTable()->declareSym(nullptr, SymbolType::IrInstruction, std::move(symData), name);
        };

        declareInst("SDIV",
                    { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                      { IrOperandType::Register, "lhs", IrOperandDir::ArgIn },
                      { IrOperandType::RegImm, "rhs", IrOperandDir::ArgIn } });

        declareInst("SAR",
                    { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                      { IrOperandType::Register, "val", IrOperandDir::ArgIn },
                      { IrOperandType::RegIntImm, "amt", IrOperandDir::ArgIn } });

        declareInst("SUB",
                    { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                      { IrOperandType::Register, "lhs", IrOperandDir::ArgIn },
                      { IrOperandType::RegImm, "rhs", IrOperandDir::ArgIn } });

        declareInst("MOV",
                    { { IrOperandType::Register, "dst", IrOperandDir::ArgOut },
                      { IrOperandType::AnyValue, "src", IrOperandDir::ArgIn } });
    }

    // Parses legalize-rule (.lrd) source into an AST using a unique source name.
    std::optional<DSL::Ast::LegalizeRuleDef::LegalizeRuleFile> parseLrd(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("test_{}.lrd", m_testId++), source);
        return ctx.parse<DSL::Parser::LegalizeRuleDef::LegalizeRuleFile, DSL::Ast::LegalizeRuleDef::LegalizeRuleFile>();
    }

    // Reads a generated file into a string for content assertions.
    std::string readFile(const std::filesystem::path &path)
    {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

  private:
    size_t m_testId{ 0 };
};

// Generates rules for an empty symbol table and verifies the boilerplate header/source and no-op result.
TEST_F(CppLegalizeRuleGeneratorTest, TestEmptyRulesGeneration)
{
    CppLegalizeRuleGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    auto headerPath = m_testTempDir / "AMD64LegalizerRules.h";
    auto sourcePath = m_testTempDir / "AMD64LegalizerRules.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string header = readFile(headerPath);
    std::string source = readFile(sourcePath);

    EXPECT_NE(header.find("#ifndef EZTARGETS_AMD64_LEGALIZER_RULES_H"), std::string::npos);
    EXPECT_NE(header.find("namespace EzTargets::AMD64Rules"), std::string::npos);
    EXPECT_NE(header.find("LegalizationResult applyRules("), std::string::npos);
    EXPECT_NE(header.find("LegalizationResult applyRuleById("), std::string::npos);

    EXPECT_NE(source.find("#include \"AMD64LegalizerRules.h\""), std::string::npos);
    EXPECT_NE(source.find("return LegalizationResult::NotModified;"), std::string::npos);
}

// Generates the SDivPow2 rule and verifies matching, predicates, transforms, emit sequence, and dispatchers.
TEST_F(CppLegalizeRuleGeneratorTest, TestSDivPow2RuleGeneration)
{
    std::string sourceText = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
        isPositiveConst($c);
    };
    emit {
        SAR i32:$dst, i32:$lhs, log2Pow2($c);
    };
};
)dsl";

    auto ast = parseLrd(sourceText);
    ASSERT_TRUE(ast.has_value());

    bool passResult = LegalizeRulePass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    ASSERT_TRUE(passResult);

    CppLegalizeRuleGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    auto headerPath = m_testTempDir / "AMD64LegalizerRules.h";
    auto sourcePath = m_testTempDir / "AMD64LegalizerRules.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string header = readFile(headerPath);
    std::string source = readFile(sourcePath);

    // Verify Header
    EXPECT_NE(header.find("LegalizationResult Rule_SDivPow2(LegalizeCtx &ctx);"), std::string::npos);
    EXPECT_NE(header.find("LegalizationResult applyRules(LegalizeCtx &ctx, MirInstructionOpCode opcode);"),
              std::string::npos);
    EXPECT_NE(header.find("LegalizationResult applyRuleById(LegalizeCtx &ctx, uint16_t ruleId);"), std::string::npos);

    // Verify Source: extern forward declarations
    EXPECT_NE(source.find("extern bool isPowTwo(int64_t arg0);"), std::string::npos);
    EXPECT_NE(source.find("extern bool isPositiveConst(int64_t arg0);"), std::string::npos);
    EXPECT_NE(source.find("extern int64_t log2Pow2(int64_t arg0);"), std::string::npos);

    // Verify Source: rule matcher
    EXPECT_NE(source.find("LegalizationResult Rule_SDivPow2(LegalizeCtx &ctx)"), std::string::npos);
    EXPECT_NE(source.find("inst->getOpCode() != MirInstructionOpCode::SDIV"), std::string::npos);
    EXPECT_NE(source.find("operands.size() != 3"), std::string::npos);
    EXPECT_NE(source.find("MirOperandType::Register"), std::string::npos);
    EXPECT_NE(source.find("MirOperandType::Integer"), std::string::npos);
    EXPECT_NE(source.find("getTotalSizeInBits() != 32"), std::string::npos);

    // Verify Source: when predicates & transforms
    EXPECT_NE(source.find("!isPowTwo("), std::string::npos);
    EXPECT_NE(source.find("!isPositiveConst("), std::string::npos);
    EXPECT_NE(source.find("log2Pow2("), std::string::npos);

    // Verify Source: emit sequence
    EXPECT_NE(source.find("MirInstructionOpCode::SAR"), std::string::npos);
    EXPECT_NE(source.find("inst->eraseFromOwner();"), std::string::npos);
    EXPECT_NE(source.find("return LegalizationResult::Legalized;"), std::string::npos);

    // Verify Source: dispatchers
    EXPECT_NE(source.find("case MirInstructionOpCode::SDIV:"), std::string::npos);
    EXPECT_NE(source.find("case 0: return Rule_SDivPow2(ctx);"), std::string::npos);
}

// Generates two rules and verifies both appear in the header and the dispatch switches.
TEST_F(CppLegalizeRuleGeneratorTest, TestMultipleRulesGeneration)
{
    std::string sourceText = R"dsl(
rule SDivPow2 {
    match {
        SDIV i32:$dst, i32:$lhs, imm(i32):$c;
    };
    when {
        isPowTwo($c);
    };
    emit {
        SAR i32:$dst, i32:$lhs, log2Pow2($c);
    };
};

rule SubZero {
    match {
        SUB i32:$dst, i32:$lhs, 0;
    };
    emit {
        MOV i32:$dst, i32:$lhs;
    };
};
)dsl";

    auto ast = parseLrd(sourceText);
    ASSERT_TRUE(ast.has_value());

    bool passResult = LegalizeRulePass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    ASSERT_TRUE(passResult);

    CppLegalizeRuleGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    std::string header = readFile(m_testTempDir / "AMD64LegalizerRules.h");
    std::string source = readFile(m_testTempDir / "AMD64LegalizerRules.cpp");

    EXPECT_NE(header.find("Rule_SDivPow2"), std::string::npos);
    EXPECT_NE(header.find("Rule_SubZero"), std::string::npos);

    EXPECT_NE(source.find("case 0: return Rule_SDivPow2(ctx);"), std::string::npos);
    EXPECT_NE(source.find("case 1: return Rule_SubZero(ctx);"), std::string::npos);

    EXPECT_NE(source.find("case MirInstructionOpCode::SDIV:"), std::string::npos);
    EXPECT_NE(source.find("case MirInstructionOpCode::SUB:"), std::string::npos);
}

// Verifies when { hasExtension("avx"); } generates a ctx.m_targetDesc->hasExtension check.
TEST_F(CppLegalizeRuleGeneratorTest, TestHasExtensionRuleGeneration)
{
    std::string sourceText = R"dsl(
rule SubWithAvx {
    match {
        SUB i32:$dst, i32:$lhs, i32:$rhs;
    };
    when {
        hasExtension("avx");
    };
    emit {
        SUB i32:$dst, i32:$lhs, i32:$rhs;
    };
};
)dsl";

    auto ast = parseLrd(sourceText);
    ASSERT_TRUE(ast.has_value());

    bool passResult = LegalizeRulePass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    ASSERT_TRUE(passResult);

    CppLegalizeRuleGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    std::string source = readFile(m_testTempDir / "AMD64LegalizerRules.cpp");

    EXPECT_NE(source.find("#include \"Descriptors/TargetDesc.h\""), std::string::npos);
    EXPECT_NE(source.find("if (!ctx.m_targetDesc || !ctx.m_targetDesc->hasExtension(\"avx\")) return LegalizationResult::NotModified;"),
              std::string::npos);
}

