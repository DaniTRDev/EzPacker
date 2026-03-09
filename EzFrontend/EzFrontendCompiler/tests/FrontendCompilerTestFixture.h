#ifndef EZPACKER_FRONTENDCOMPILERTESTFIXTURE_H
#define EZPACKER_FRONTENDCOMPILERTESTFIXTURE_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilerDriver.h"
#include "FrontendCompilationUnit.h"
#include "FrontendCompilationUnitPhase.h"
#include "CompilationPhases/AstLowering.h"
#include "CompilationPhases/IncludePhase.h"
#include "CompilationPhases/Parsing.h"
#include "CompilationPhases/SemanticAnalysis.h"
#include "CompilationPhases/Tokenization.h"
#include "CompilationPhases/Semantic/SymbolAndTypeResolver.h"
#include "CompilationPhases/Semantic/SymbolDefinition.h"
#include "CompilationPhases/Semantic/TypeCheck.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

/**
 * Test fixture for the EzFrontendCompiler module.
 * Provides shared infrastructure (logger, source manager, error collector, compiler driver)
 * and helper methods for running individual phases or the full pipeline.
 */
class FrontendCompilerTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    // ──────────────────────────────────────────────────────────────
    //  Compilation unit helpers
    // ──────────────────────────────────────────────────────────────

    /**
     * Creates a FrontendCompilationUnit from inline source content.
     * Registers the source in the source manager and calls create().
     * @param content  Source code string.
     * @param name     Logical name for the source.
     * @return shared_ptr to the created compilation unit, or nullptr on failure.
     */
    std::shared_ptr<FrontendCompilationUnit> createUnitFromSource(const std::string &content,
                                                                   const std::string &name = "TEST_SOURCE");

    // ──────────────────────────────────────────────────────────────
    //  Phase runners (on a single unit)
    // ──────────────────────────────────────────────────────────────

    /** Runs only the TokenizationPhase on the given unit. */
    bool runTokenization(FrontendCompilationUnit *unit);

    /** Runs Tokenization + Parsing on the given unit. */
    bool runUpToParsing(FrontendCompilationUnit *unit);

    /** Runs Tokenization + Parsing + SemanticAnalysis (all 3 sub-phases) on the given unit. */
    bool runUpToSemantics(FrontendCompilationUnit *unit);

    /** Runs the full single-unit pipeline: Tokenization → Parsing → Semantic → AstLowering. */
    bool runFullSingleUnit(FrontendCompilationUnit *unit);

    // ──────────────────────────────────────────────────────────────
    //  Compiler driver helpers
    // ──────────────────────────────────────────────────────────────

    /**
     * Adds inline source to the driver and compiles everything.
     * @param content  Source code string.
     * @param name     Logical name for the source.
     * @return true if addSource + compile both succeed.
     */
    bool compileFromSource(const std::string &content, const std::string &name = "TEST_SOURCE");

    /**
     * Adds an .ez file from the programs/ subfolder to the driver and compiles it.
     * @param fileName  File name relative to the programs/ folder.
     * @return true if addSourceFromFile + compile both succeed.
     */
    bool compileFromFile(const std::string &fileName);

    // ──────────────────────────────────────────────────────────────
    //  Accessors
    // ──────────────────────────────────────────────────────────────

    std::shared_ptr<ErrorCollector> getErrorCollector() const;
    std::shared_ptr<SourceManager> getSourceManager() const;
    FrontendCompilerDriver *getDriver() const;

    /** Returns true if the error collector has any fatal errors in the current scope. */
    bool hasFatalErrors() const;

    /** Returns the path to the programs/ directory. */
    static std::filesystem::path getProgramsDir();

  protected:
    std::shared_ptr<SyncLogger> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<FrontendCompilerDriver> m_driver;
};

#endif // EZPACKER_FRONTENDCOMPILERTESTFIXTURE_H

