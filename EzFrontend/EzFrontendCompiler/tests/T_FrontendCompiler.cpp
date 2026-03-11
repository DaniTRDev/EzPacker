#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdlib>
#include "FrontendCompilerDriver.h"
#include "EzSemantics.h"

namespace fs = std::filesystem;

class FrontendCompilerTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<FrontendCompilerDriver> driver;
    fs::path tempDir;

    void SetUp() override
    {
        // Create a unique temp directory for this test run
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        tempDir = fs::temp_directory_path() / ("EzFrontendCompilerTests_" + std::to_string(timestamp));

        if (fs::exists(tempDir))
        {
            fs::remove_all(tempDir);
        }
        fs::create_directories(tempDir);

        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(tempDir);
        driver = std::make_shared<FrontendCompilerDriver>(ec, sm);
        ec->beginScope();
    }

    void TearDown() override
    {
        if (fs::exists(tempDir))
        {
            fs::remove_all(tempDir);
        }

        ec->endScope(ErrorAction::Commit);
    }

    std::string createTempFile(const std::string &filename, const std::string &content)
    {
        fs::path filePath = tempDir / filename;
        std::ofstream ofs(filePath);
        ofs << content;
        ofs.close();
        return filename; // Return filename relative to tempDir
    }
};

TEST_F(FrontendCompilerTests, SingleFileCompilation)
{
    std::string src = "void main() { nop; }";
    driver->addSource(src, "main.ez");
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTests, MultiFileCompilation_Include)
{
    // lib.ez
    createTempFile("lib.ez", "void foo() { nop; }");

    // main.ez
    std::string mainSrc = "include <\"lib.ez\"> void main() { nop; }";
    driver->addSource(mainSrc, "main.ez");

    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTests, MultiFileCompilation_MissingFile)
{
    std::string mainSrc = "include <\"missing.ez\"> void main() { nop; }";
    driver->addSource(mainSrc, "main.ez");

    EXPECT_FALSE(driver->compile());
}

TEST_F(FrontendCompilerTests, MultiFileCompilation_CircularDependency)
{
    // a.ez includes b.ez
    createTempFile("a.ez", "include <\"b.ez\"> void a() { nop; }");
    // b.ez includes a.ez
    createTempFile("b.ez", "include <\"a.ez\"> void b() { nop; }");

    // Start with a.ez
    EXPECT_TRUE(driver->addSourceFromFile("a.ez"));
    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTests, SymbolRedefinition_ExternalRef)
{
    std::string mainSrc = "include <\"lib.ez\"> void foo() { call %ExternalModule(); }";

    createTempFile("lib.ez", "void ExternalModule() { nop; }");
    driver->addSource(mainSrc, "main.ez");

    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTests, SymbolRedefinition_CrossModule)
{
    std::string mainSrc = "include <\"lib.ez\"> void foo() { nop; }";

    createTempFile("lib.ez", "void foo() { nop; }");
    driver->addSource(mainSrc, "main.ez");

    // Should fail because foo is already defined in lib.ez
    EXPECT_FALSE(driver->compile());
}
