#include "EzDslTestSuite.h"
#include "Ast/InstructionDefLangAst.h"
#include "Ast/InstructionSelDefLangAst.h"
#include "CodeGenerators/CppISelTableGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/InstructionSelDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/InstSelPass.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

class ISelTableGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_tempDir =
                std::filesystem::temp_directory_path() / ("ezdsl_isel_test_" + std::to_string(std::random_device{}()));
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

TEST_F(ISelTableGeneratorTest, GeneratesISelTableAndMatchers)
{
    std::string isfDsl = R"dsl(
pattern SelectAdd {
    match {
        ADD $dst, GPR:$lhs, GPR:$rhs;
    };
    emit {
        ADD_r64_r64 $dst, GPR:$lhs, GPR:$rhs;
    };
    cost(1);
};
)dsl";

    SymbolTable symTable(getAllocator());

    // Setup Mock Environment
    Sema::Symbols::RegisterClassSymbol gprClass{ .m_name = "GPR",
                                                 .m_bankId = InvalidSymbolId,
                                                 .m_registers = std::pmr::vector<SymbolId>{ getAllocator() } };
    symTable.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::RegisterClass, gprClass, "GPR");
    Symbol *gprSym = symTable.getSymByName("GPR");
    SymbolId gprId = gprSym ? gprSym->getId() : InvalidSymbolId;

    std::pmr::vector<Sema::Symbols::IrOperandSymbol> irOps{ getAllocator() };
    for (int i = 0; i < 3; ++i)
    {
        irOps.push_back(Sema::Symbols::IrOperandSymbol{ .m_name = "op" });
    }
    Sema::Symbols::IrInstructionSymbol irInst{ .m_name = "ADD", .m_operands = std::move(irOps) };
    symTable.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::IrInstruction, irInst, "ADD");

    std::pmr::vector<Sema::Symbols::TargetOperandSymbol> targetOps{ getAllocator() };
    targetOps.push_back(Sema::Symbols::TargetOperandSymbol{ .m_kind = DSL::Ast::InstDef::InstOperandKind::Register,
                                                            .m_typeOrClassId = gprId,
                                                            .m_name = "dst",
                                                            .m_dir = DSL::Ast::InstDef::InstOperandDir::ArgOut });
    targetOps.push_back(Sema::Symbols::TargetOperandSymbol{ .m_kind = DSL::Ast::InstDef::InstOperandKind::Register,
                                                            .m_typeOrClassId = gprId,
                                                            .m_name = "lhs",
                                                            .m_dir = DSL::Ast::InstDef::InstOperandDir::ArgIn });
    targetOps.push_back(Sema::Symbols::TargetOperandSymbol{ .m_kind = DSL::Ast::InstDef::InstOperandKind::Register,
                                                            .m_typeOrClassId = gprId,
                                                            .m_name = "rhs",
                                                            .m_dir = DSL::Ast::InstDef::InstOperandDir::ArgIn });

    Sema::Symbols::TargetInstructionSymbol targetInst{ .m_name = "ADD_r64_r64", .m_args = std::move(targetOps) };
    symTable.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Instruction, targetInst, "ADD_r64_r64");

    ParseContext isfCtx = createParseContextFromBuff("rules.isf", isfDsl);
    auto isfAst = isfCtx.parse<DSL::Parser::InstSelDef::ISelDefFileParser, DSL::Ast::InstSelDef::ISelDefFile>();
    ASSERT_TRUE(isfAst.has_value());

    ASSERT_TRUE(InstSelPass::run(getDiagCollector(), &symTable, &*isfAst));

    ASSERT_TRUE(CodeGenerators::GenerateTargetISelTable(getDiagCollector(),
                                                       &symTable,
                                                       m_tempDir,
                                                       "x86_64",
                                                       CodeGenerators::ISelTableGenWorkingMode::Full));

    auto headerPath = m_tempDir / "x86_64ISelTable.h";
    auto sourcePath = m_tempDir / "x86_64ISelTable.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFile(headerPath);
    EXPECT_NE(headerContent.find("class x86_64InstructionSelector"), std::string::npos);
    EXPECT_NE(headerContent.find("select(MirBuilderContext *ctx, MirInstruction *inst)"), std::string::npos);

    std::string sourceContent = readFile(sourcePath);
    EXPECT_NE(sourceContent.find("x86_64InstructionSelector::select"), std::string::npos);
}
