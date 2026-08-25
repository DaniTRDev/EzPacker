#include "EzDslTestSuite.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "CodeGenerators/CppMirInstructionGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/IrInstructionPass.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <thread>

/**
 * Test fixture for C++ MIR instruction set definition code generator (CppMirInstructionGenerator).
 * Verifies code generation, operand constraint arrays, instruction flag bitmasks,
 * write-if-changed timestamp caching, and null safety.
 */
class MirIrInstructionGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    /**
     * Initializes test environment and creates a temporary sandbox directory for output code generation.
     */
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();

        m_tempDir =
                std::filesystem::temp_directory_path() / ("ezdsl_ir_test_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(m_tempDir);

        getDiagCollector()->trace("MirIrInstructionGeneratorTest", "Testing dir at: {}", m_tempDir.string());
    }

    /**
     * Cleans up the temporary sandbox directory and tears down the test environment.
     */
    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(m_tempDir, ec);

        DslTestSuiteAsGtest::TearDown();
    }

    /**
     * Reads and returns the entire contents of a file on disk as a string.
     */
    std::string readFile(const std::filesystem::path &filePath) const
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return {};
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::filesystem::path m_tempDir;
};

// ============================================================================
// 1. Full Instruction Set Generation & Content Verification
// ============================================================================

/**
 * Verifies end-to-end code generation of MirInstructionSetDefs.h from IR instruction definitions across multiple categories,
 * checking macro guards, tier macros, operand constraints, and instruction flag bitmasks.
 */
TEST_F(MirIrInstructionGeneratorTest, GeneratesInstructionDefsWithMultipleCategories)
{
    std::string dslContent = R"dsl(
ir_inst NOP() {
    CATEGORY(System);
    TIER(HighLevel);
}

ir_inst MOV(Register:dst OUT, AnyValue:src IN) {
    CATEGORY(DataMovement);
    TIER(HighLevel);
}

ir_inst LOAD(Register:dst OUT, AddressSource:src IN) {
    CATEGORY(Memory);
    TIER(HighLevel);
    FLAGS(ReadsMemory);
}

ir_inst STORE(AddressSource:dst IN, AnyValue:src IN) {
    CATEGORY(Memory);
    TIER(HighLevel);
    FLAGS(WritesMemory, HasSideEffect);
}

ir_inst ADD(Register:dst OUT, Register:lhs IN, RegImm:rhs IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
    FLAGS(SizeMatch, IsCommutative);
}

ir_inst JMP(Reference:target IN) {
    CATEGORY(ControlFlow);
    TIER(HighLevel);
    FLAGS(IsTerminator, IsBranch);
}

ir_inst POP_RET(Register:token IN, Register:dst OUT) {
    CATEGORY(DataMovement);
    TIER(PassInternal);
    FLAGS(HasSideEffect);
}
)dsl";

    // 1. Parse .irdf DSL
    ParseContext parseCtx = createParseContextFromBuff("instructions.irdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    ASSERT_TRUE(ast.has_value());

    // 2. Run Semantic Analysis Pass
    SymbolTable symTable(getAllocator());
    IrInstructionPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    // 3. Generate Header Definition File
    bool success = CodeGenerators::GenerateMirIrInstructionDefs(getDiagCollector(), &symTable, m_tempDir);
    ASSERT_TRUE(success);

    auto defsPath = m_tempDir / "MirInstructionSetDefs.h";
    ASSERT_TRUE(std::filesystem::exists(defsPath));

    std::string content = readFile(defsPath);

    // Verify macro boilerplate and safety wrappers
    EXPECT_NE(content.find("#ifdef INSTRUCTION"), std::string::npos);
    EXPECT_NE(content.find("#define OPERAND_CONSTRAINTS(...)"), std::string::npos);
    EXPECT_NE(content.find("#define F(x) MirInstructionFlags::x"), std::string::npos);
    EXPECT_NE(content.find("#define T(x) MirInstructionTier::x"), std::string::npos);
    EXPECT_NE(content.find("INSTRUCTION(INVALID, T(HighLevel), MirCat_Invalid, OPERAND_CONSTRAINTS(), F(None))"),
              std::string::npos);
    EXPECT_NE(content.find("#undef OPERAND_CONSTRAINTS"), std::string::npos);
    EXPECT_NE(content.find("#endif // INSTRUCTION"), std::string::npos);

    // Verify NOP generation (Zero operands)
    EXPECT_NE(content.find("INSTRUCTION(NOP, T(HighLevel), MirCat_System, OPERAND_CONSTRAINTS(), F(None))"),
              std::string::npos);

    // Verify MOV generation
    EXPECT_NE(content.find("INSTRUCTION(MOV,\n"
                           "            T(HighLevel),\n"
                           "            MirCat_DataMovement,\n"
                           "            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },\n"
                           "                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),\n"
                           "            F(None))"),
              std::string::npos);

    // Verify LOAD generation
    EXPECT_NE(content.find(
                      "INSTRUCTION(LOAD,\n"
                      "            T(HighLevel),\n"
                      "            MirCat_Memory,\n"
                      "            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },\n"
                      "                                { ExpectedOperandType::AddressSource, MirOperandFlag::Read }),\n"
                      "            F(ReadsMemory))"),
              std::string::npos);

    // Verify STORE generation
    EXPECT_NE(content.find(
                      "INSTRUCTION(STORE,\n"
                      "            T(HighLevel),\n"
                      "            MirCat_Memory,\n"
                      "            OPERAND_CONSTRAINTS({ ExpectedOperandType::AddressSource, MirOperandFlag::Read },\n"
                      "                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),\n"
                      "            F(WritesMemory) | F(HasSideEffect))"),
              std::string::npos);

    // Verify ADD generation
    EXPECT_NE(content.find("INSTRUCTION(ADD,\n"
                           "            T(HighLevel),\n"
                           "            MirCat_Arithmetic,\n"
                           "            OPERAND_CONSTRAINTS({ ExpectedOperandType::Register, MirOperandFlag::Write },\n"
                           "                                { ExpectedOperandType::Register, MirOperandFlag::Read },\n"
                           "                                { ExpectedOperandType::RegImm, MirOperandFlag::Read }),\n"
                           "            F(SizeMatch) | F(IsCommutative))"),
              std::string::npos);

    // Verify JMP generation (Single operand constraint formatting)
    EXPECT_NE(
            content.find("INSTRUCTION(JMP,\n"
                         "            T(HighLevel),\n"
                         "            MirCat_ControlFlow,\n"
                         "            OPERAND_CONSTRAINTS({ ExpectedOperandType::Reference, MirOperandFlag::Read }),\n"
                         "            F(IsTerminator) | F(IsBranch))"),
            std::string::npos);

    // Verify POP_RET generation (PassInternal tier)
    EXPECT_NE(content.find("INSTRUCTION(POP_RET,\n"
                           "            T(PassInternal),\n"
                           "            MirCat_DataMovement,\n"),
              std::string::npos);
}

// ============================================================================
// 2. Direct File Path Output Specification
// ============================================================================

/**
 * Verifies code generation when targeting an explicit file path rather than a directory.
 */
TEST_F(MirIrInstructionGeneratorTest, GeneratesToExplicitFilePath)
{
    std::string dslContent = R"dsl(
ir_inst HALT() {
    CATEGORY(System);
    TIER(HighLevel);
    FLAGS(IsTerminator, HasSideEffect);
}
)dsl";

    ParseContext parseCtx = createParseContextFromBuff("instructions.irdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    ASSERT_TRUE(ast.has_value());

    SymbolTable symTable(getAllocator());
    IrInstructionPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    auto customFilePath = m_tempDir / "CustomMirDefs.h";
    bool success = CodeGenerators::GenerateMirIrInstructionDefs(getDiagCollector(), &symTable, customFilePath);

    ASSERT_TRUE(success);
    ASSERT_TRUE(std::filesystem::exists(customFilePath));

    std::string content = readFile(customFilePath);
    EXPECT_NE(content.find("INSTRUCTION(HALT, T(HighLevel), MirCat_System, OPERAND_CONSTRAINTS(), F(IsTerminator) | "
                           "F(HasSideEffect))"),
              std::string::npos);
}

// ============================================================================
// 3. Incremental Build: Write-If-Changed Verification
// ============================================================================

/**
 * Verifies write-if-changed optimization ensuring file timestamps remain untouched when contents are identical.
 */
TEST_F(MirIrInstructionGeneratorTest, PreservesTimestampWhenContentIsUnchanged)
{
    std::string dslContent = R"dsl(
ir_inst NOP() {
    CATEGORY(System);
    TIER(HighLevel);
}
)dsl";

    ParseContext parseCtx = createParseContextFromBuff("instructions.irdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    ASSERT_TRUE(ast.has_value());

    SymbolTable symTable(getAllocator());
    IrInstructionPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    auto targetPath = m_tempDir / "MirInstructionSetDefs.h";

    // First generation
    ASSERT_TRUE(CodeGenerators::GenerateMirIrInstructionDefs(getDiagCollector(), &symTable, m_tempDir));
    ASSERT_TRUE(std::filesystem::exists(targetPath));
    auto initialTimestamp = std::filesystem::last_write_time(targetPath);

    // Sleep to ensure time difference if file gets touched
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Second generation with identical symbol data
    ASSERT_TRUE(CodeGenerators::GenerateMirIrInstructionDefs(getDiagCollector(), &symTable, m_tempDir));
    auto secondTimestamp = std::filesystem::last_write_time(targetPath);

    EXPECT_EQ(initialTimestamp, secondTimestamp);
}

// ============================================================================
// 4. Baseline & Error Handling Tests
// ============================================================================

/**
 * Verifies that code generator handles an empty symbol table gracefully by outputting baseline boilerplate.
 */
TEST_F(MirIrInstructionGeneratorTest, HandlesEmptySymbolTableGracefully)
{
    SymbolTable emptyTable(getAllocator());

    bool success = CodeGenerators::GenerateMirIrInstructionDefs(getDiagCollector(), &emptyTable, m_tempDir);
    ASSERT_TRUE(success);

    auto defsPath = m_tempDir / "MirInstructionSetDefs.h";
    ASSERT_TRUE(std::filesystem::exists(defsPath));

    std::string content = readFile(defsPath);
    EXPECT_NE(content.find("INSTRUCTION(INVALID, T(HighLevel), MirCat_Invalid, OPERAND_CONSTRAINTS(), F(None))"),
              std::string::npos);
}

/**
 * Verifies that the generator rejects null pointers for diagnostics or symbol table inputs.
 */
TEST_F(MirIrInstructionGeneratorTest, FailsGracefullyOnNullInputs)
{
    DiagnosticCollector collector;
    SymbolTable symTable(getAllocator());

    EXPECT_FALSE(CodeGenerators::GenerateMirIrInstructionDefs(&collector, nullptr, m_tempDir));
    EXPECT_FALSE(CodeGenerators::GenerateMirIrInstructionDefs(nullptr, &symTable, m_tempDir));
}