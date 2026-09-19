#include "EzDslCodeGeneratorsTestSuite.h"
#include "CodeGenerators/CodeGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/SymbolTable.h"

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace CodeGenerators;

/**
 * Concrete dummy generator to test protected helpers and lifecycle in CodeGenerator base class.
 */
class TestableCodeGenerator : public CodeGenerator
{
  public:
    TestableCodeGenerator(std::string_view name,
                          DiagnosticCollector *collector,
                          SymbolTable *table,
                          std::filesystem::path outPath) : CodeGenerator(name, collector, table, std::move(outPath))
    {
    }

    bool run() override
    {
        if (!validate())
        {
            return false;
        }

        auto targetFile = resolveSingleFilePath("Output.txt");
        return writeOutput(targetFile, "Generated Content");
    }

    using CodeGenerator::error;
    using CodeGenerator::resolveHeaderAndSourcePaths;
    using CodeGenerator::resolveSingleFilePath;
    using CodeGenerator::trace;
    using CodeGenerator::validate;
    using CodeGenerator::warn;
    using CodeGenerator::writeOutput;
};

/**
 * Fixture exposing the concrete TestableCodeGenerator to exercise CodeGenerator base-class helpers.
 */
class CodeGeneratorBaseTest : public EzDslCodeGeneratorsTestSuiteAsGtest
{
};

// ============================================================================
// 1. Validation & Null Safety Tests
// ============================================================================

TEST_F(CodeGeneratorBaseTest, TestValidationFailsOnNullPointers)
{
    TestableCodeGenerator genNullCollector("TestGen", nullptr, getSymbolTable(), m_testTempDir);
    EXPECT_FALSE(genNullCollector.validate());

    TestableCodeGenerator genNullTable("TestGen", getDiagCollector(), nullptr, m_testTempDir);
    EXPECT_FALSE(genNullTable.validate());

    TestableCodeGenerator genEmptyPath("TestGen", getDiagCollector(), getSymbolTable(), "");
    EXPECT_FALSE(genEmptyPath.validate());

    TestableCodeGenerator genValid("TestGen", getDiagCollector(), getSymbolTable(), m_testTempDir);
    EXPECT_TRUE(genValid.validate());
}

// Verifies the base-class accessors return the configured name, collector, symbol table, and path.
TEST_F(CodeGeneratorBaseTest, TestAccessors)
{
    TestableCodeGenerator gen("CustomGenerator", getDiagCollector(), getSymbolTable(), m_testTempDir);
    EXPECT_EQ(gen.getGeneratorName(), "CustomGenerator");
    EXPECT_EQ(gen.getCollector(), getDiagCollector());
    EXPECT_EQ(gen.getSymbolTable(), getSymbolTable());
    EXPECT_EQ(gen.getOutputPath(), m_testTempDir);
}

// ============================================================================
// 2. Path Resolution Tests
// ============================================================================

TEST_F(CodeGeneratorBaseTest, TestSingleFilePathResolution)
{
    // Case 1: Existing directory
    TestableCodeGenerator genDir("TestGen", getDiagCollector(), getSymbolTable(), m_testTempDir);
    EXPECT_EQ(genDir.resolveSingleFilePath("Default.h"), m_testTempDir / "Default.h");

    // Case 2: Extensionless path (treated as directory)
    auto nonExistentDir = m_testTempDir / "subfolder";
    TestableCodeGenerator genExtLess("TestGen", getDiagCollector(), getSymbolTable(), nonExistentDir);
    EXPECT_EQ(genExtLess.resolveSingleFilePath("Default.h"), nonExistentDir / "Default.h");

    // Case 3: Explicit file path with extension
    auto explicitFile = m_testTempDir / "CustomOutput.gen.h";
    TestableCodeGenerator genFile("TestGen", getDiagCollector(), getSymbolTable(), explicitFile);
    EXPECT_EQ(genFile.resolveSingleFilePath("Default.h"), explicitFile);
}

// Verifies header/source path pairing for directory, .h, .hpp, .cpp, and .cxx output paths.
TEST_F(CodeGeneratorBaseTest, TestHeaderAndSourcePathsResolution)
{
    // Case 1: Directory or extensionless output path
    TestableCodeGenerator genDir("TestGen", getDiagCollector(), getSymbolTable(), m_testTempDir);
    auto pathsDir = genDir.resolveHeaderAndSourcePaths("MyModule");
    EXPECT_EQ(pathsDir.m_headerPath, m_testTempDir / "MyModule.h");
    EXPECT_EQ(pathsDir.m_sourcePath, m_testTempDir / "MyModule.cpp");

    // Case 2: Explicit .h file
    auto headerPath = m_testTempDir / "CustomHeader.h";
    TestableCodeGenerator genH("TestGen", getDiagCollector(), getSymbolTable(), headerPath);
    auto pathsH = genH.resolveHeaderAndSourcePaths("Ignored");
    EXPECT_EQ(pathsH.m_headerPath, headerPath);
    EXPECT_EQ(pathsH.m_sourcePath, m_testTempDir / "CustomHeader.cpp");

    // Case 3: Explicit .hpp file
    auto hppPath = m_testTempDir / "CustomHpp.hpp";
    TestableCodeGenerator genHpp("TestGen", getDiagCollector(), getSymbolTable(), hppPath);
    auto pathsHpp = genHpp.resolveHeaderAndSourcePaths("Ignored");
    EXPECT_EQ(pathsHpp.m_headerPath, hppPath);
    EXPECT_EQ(pathsHpp.m_sourcePath, m_testTempDir / "CustomHpp.cpp");

    // Case 4: Explicit .cpp file
    auto sourcePath = m_testTempDir / "CustomSource.cpp";
    TestableCodeGenerator genCpp("TestGen", getDiagCollector(), getSymbolTable(), sourcePath);
    auto pathsCpp = genCpp.resolveHeaderAndSourcePaths("Ignored");
    EXPECT_EQ(pathsCpp.m_sourcePath, sourcePath);
    EXPECT_EQ(pathsCpp.m_headerPath, m_testTempDir / "CustomSource.h");

    // Case 5: Explicit .cxx file
    auto cxxPath = m_testTempDir / "CustomCxx.cxx";
    TestableCodeGenerator genCxx("TestGen", getDiagCollector(), getSymbolTable(), cxxPath);
    auto pathsCxx = genCxx.resolveHeaderAndSourcePaths("Ignored");
    EXPECT_EQ(pathsCxx.m_sourcePath, cxxPath);
    EXPECT_EQ(pathsCxx.m_headerPath, m_testTempDir / "CustomCxx.h");
}

// ============================================================================
// 3. Atomic File Writing & Timestamp Preservation
// ============================================================================

TEST_F(CodeGeneratorBaseTest, TestWriteFileIfChangedNewFileAndParentDirCreation)
{
    auto nestedFile = m_testTempDir / "nested" / "sub" / "dir" / "new_file.txt";
    std::string content = "Testing atomic write with automatic directory creation.";

    EXPECT_TRUE(CodeGenerator::WriteFileIfChanged(nestedFile, content));
    EXPECT_TRUE(std::filesystem::exists(nestedFile));
    EXPECT_EQ(readFileContent(nestedFile), content);
}

// Verifies unchanged content preserves the timestamp while changed content updates content and mtime.
TEST_F(CodeGeneratorBaseTest, TestWriteFileIfChangedPreservesMtime)
{
    auto testFile = m_testTempDir / "mtime_file.txt";
    std::string content1 = "Original generated content.";

    EXPECT_TRUE(CodeGenerator::WriteFileIfChanged(testFile, content1));
    EXPECT_TRUE(std::filesystem::exists(testFile));

    // Explicitly backdate file modification time by 5 seconds
    auto pastTime = std::filesystem::last_write_time(testFile) - std::chrono::seconds(5);
    std::filesystem::last_write_time(testFile, pastTime);

    auto mtime1 = std::filesystem::last_write_time(testFile);

    // Writing identical content must preserve the modification timestamp
    EXPECT_TRUE(CodeGenerator::WriteFileIfChanged(testFile, content1));
    auto mtime2 = std::filesystem::last_write_time(testFile);
    EXPECT_EQ(mtime1, mtime2);

    // Writing modified content must update the file content and mtime
    std::string content2 = "Modified generated content with changes.";
    EXPECT_TRUE(CodeGenerator::WriteFileIfChanged(testFile, content2));
    auto mtime3 = std::filesystem::last_write_time(testFile);
    EXPECT_NE(mtime1, mtime3);
    EXPECT_EQ(readFileContent(testFile), content2);
}

// Verifies WriteFileIfChanged reports an error when a parent path component is an ordinary file.
TEST_F(CodeGeneratorBaseTest, TestWriteFileIfChangedErrorOnInvalidPath)
{
    // Create a regular file
    auto regularFile = m_testTempDir / "not_a_directory.txt";
    EXPECT_TRUE(CodeGenerator::WriteFileIfChanged(regularFile, "regular file"));

    // Attempt to create a file where 'not_a_directory.txt' is treated as a parent directory
    auto invalidNestedPath = regularFile / "sub_file.txt";
    std::string errorOut;
    EXPECT_FALSE(CodeGenerator::WriteFileIfChanged(invalidNestedPath, "content", &errorOut));
    EXPECT_FALSE(errorOut.empty());
}

// ============================================================================
// 4. Execution and Diagnostic Helpers
// ============================================================================

TEST_F(CodeGeneratorBaseTest, TestWriteOutputAndDiagnosticEmission)
{
    TestableCodeGenerator gen("TestGen", getDiagCollector(), getSymbolTable(), m_testTempDir);
    EXPECT_TRUE(gen.run());

    auto generatedFile = m_testTempDir / "Output.txt";
    EXPECT_TRUE(std::filesystem::exists(generatedFile));
    EXPECT_EQ(readFileContent(generatedFile), "Generated Content");

    // Test manual diagnostic emissions
    gen.trace("Diagnostic trace message: {}", 123);
    gen.warn("Diagnostic warning message: {}", "warning_detail");
    gen.error("Diagnostic error message: {}", "error_detail");
}
