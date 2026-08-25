#include "EzDslTestSuite.h"
#include "Ast/TypeDefLangAst.h"
#include "CodeGenerators/CppMirTypeTableGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TypeDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/TypePass.h"

#include <random>

/**
 * Test fixture for C++ MIR Type Table code generator (CppMirTypeTableGenerator).
 * Verifies generation of MirTypeTable.h and MirTypeTable.cpp, working mode isolation
 * (Header-Only, Source-Only, Full), and null safety.
 */
class MirTypeTableGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    /**
     * Sets up the test environment and creates a temporary sandbox directory.
     */
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();

        m_tempDir = std::filesystem::temp_directory_path() / ("ezdsl_test_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(m_tempDir);

        getDiagCollector()->trace("MirTypeTableGeneratorTest", "Testing dir at: {}", m_tempDir.string());
    }

    /**
     * Cleans up the temporary sandbox directory after test execution.
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
// 1. Full Generation & Content Verification
// ============================================================================

/**
 * Verifies end-to-end code generation of both MirTypeTable.h and MirTypeTable.cpp from .tyf DSL input,
 * asserting header declarations, member pointers, accessor methods, and initialize() instantiations.
 */
TEST_F(MirTypeTableGeneratorTest, GeneratesHeaderAndSourceWithValidTypes)
{
    std::string dslContent = R"(
        void _void;
        bindingToken __bindToken;
        integer i8(8);
        integer i32(32);
        float f32(32);
        float f64(64);
        pointer ptr;
    )";

    // 1. Parse .tyf DSL
    ParseContext parseCtx = createParseContextFromBuff("types.tyf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(ast.has_value());

    // 2. Run Semantic Analysis
    SymbolTable symTable(getAllocator());

    TypePass typePass;
    ASSERT_TRUE(typePass.run(getDiagCollector(), &symTable, &*ast));

    // 3. Generate Type Table
    bool success = CodeGenerators::GenerateMirTypeTable(getDiagCollector(),
                                                        &symTable,
                                                        m_tempDir,
                                                        CodeGenerators::MirTypeTableGenWorkingMode::Full);

    ASSERT_TRUE(success);

    auto headerPath = m_tempDir / "MirTypeTable.h";
    auto sourcePath = m_tempDir / "MirTypeTable.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFile(headerPath);
    std::string sourceContent = readFile(sourcePath);

    // Verify Header method declarations
    EXPECT_NE(headerContent.find("MirType *_void();"), std::string::npos);
    EXPECT_NE(headerContent.find("MirType *i8();"), std::string::npos);
    EXPECT_NE(headerContent.find("MirType *i32();"), std::string::npos);
    EXPECT_NE(headerContent.find("MirType *f32();"), std::string::npos);
    EXPECT_NE(headerContent.find("MirType *f64();"), std::string::npos);

    // Verify Header member fields
    EXPECT_NE(headerContent.find("MirType *m_i8Type{ nullptr };"), std::string::npos);
    EXPECT_NE(headerContent.find("MirType *m_i32Type{ nullptr };"), std::string::npos);
    EXPECT_NE(headerContent.find("MirType *m_f32Type{ nullptr };"), std::string::npos);

    // Verify Source getter implementations
    EXPECT_NE(sourceContent.find("MirType *MirTypeTable::i32() { return m_i32Type; }"), std::string::npos);
    EXPECT_NE(sourceContent.find("MirType *MirTypeTable::f64() { return m_f64Type; }"), std::string::npos);

    // Verify initialize() instantiation calls
    EXPECT_NE(sourceContent.find("m__voidType = create(MirTypeKind::Void, 0, {}, \"_void\");"), std::string::npos);
    EXPECT_NE(sourceContent.find("m_i8Type = create(MirTypeKind::Integer, 8, {}, \"i8\");"), std::string::npos);
    EXPECT_NE(sourceContent.find("m_i32Type = create(MirTypeKind::Integer, 32, {}, \"i32\");"), std::string::npos);
    EXPECT_NE(sourceContent.find("m_f32Type = create(MirTypeKind::FloatingPoint, 32, {}, \"f32\");"),
              std::string::npos);
}

// ============================================================================
// 2. Working Mode Isolation (Header-Only / Source-Only)
// ============================================================================

/**
 * Verifies Header-Only generation mode, creating MirTypeTable.h while omitting MirTypeTable.cpp.
 */
TEST_F(MirTypeTableGeneratorTest, GeneratesHeaderOnlyWhenRequested)
{
    std::string dslContent = "integer i32(32);";
    ParseContext parseCtx = createParseContextFromBuff("types.tyf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(ast.has_value());

    DiagnosticCollector collector;
    std::pmr::monotonic_buffer_resource arena;
    SymbolTable symTable(&arena);

    TypePass typePass;
    ASSERT_TRUE(typePass.run(&collector, &symTable, &*ast));

    bool success = CodeGenerators::GenerateMirTypeTable(&collector,
                                                        &symTable,
                                                        m_tempDir,
                                                        CodeGenerators::MirTypeTableGenWorkingMode::Header);

    ASSERT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "MirTypeTable.h"));
    EXPECT_FALSE(std::filesystem::exists(m_tempDir / "MirTypeTable.cpp"));
}

/**
 * Verifies Source-Only generation mode, creating MirTypeTable.cpp while omitting MirTypeTable.h.
 */
TEST_F(MirTypeTableGeneratorTest, GeneratesSourceOnlyWhenRequested)
{
    std::string dslContent = "integer i32(32);";
    ParseContext parseCtx = createParseContextFromBuff("types.tyf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(ast.has_value());

    DiagnosticCollector collector;
    std::pmr::monotonic_buffer_resource arena;
    SymbolTable symTable(&arena);

    TypePass typePass;
    ASSERT_TRUE(typePass.run(&collector, &symTable, &*ast));

    bool success = CodeGenerators::GenerateMirTypeTable(&collector,
                                                        &symTable,
                                                        m_tempDir,
                                                        CodeGenerators::MirTypeTableGenWorkingMode::Source);

    ASSERT_TRUE(success);
    EXPECT_FALSE(std::filesystem::exists(m_tempDir / "MirTypeTable.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "MirTypeTable.cpp"));
}

// ============================================================================
// 3. Error Handling
// ============================================================================

/**
 * Verifies that the type table generator fails gracefully when provided a nullptr symbol table.
 */
TEST_F(MirTypeTableGeneratorTest, FailsGracefullyOnNullSymbolTable)
{
    DiagnosticCollector collector;
    bool success = CodeGenerators::GenerateMirTypeTable(&collector,
                                                        nullptr,
                                                        m_tempDir,
                                                        CodeGenerators::MirTypeTableGenWorkingMode::Full);

    EXPECT_FALSE(success);
}