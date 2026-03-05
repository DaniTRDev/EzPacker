#ifndef EZPACKER_FRONTENDCOMPILERTESTFIXTURE_H
#define EZPACKER_FRONTENDCOMPILERTESTFIXTURE_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnit.h"
#include "FrontendCompilationUnitPhase.h"
#include "FrontendCompilerDriver.h"
#include "CompilationPhases/AstLowering.h"
#include "CompilationPhases/Parsing.h"
#include "CompilationPhases/SemanticAnalysis.h"
#include "CompilationPhases/Tokenization.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

/**
 * Test fixture for the EzFrontendCompiler library. Provides shared infrastructure (error collector, source manager,
 * logger) and helpers to run individual phases or the full driver on inline source strings and .ez files on disk.
 */
class FrontendCompilerTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    // ──────────────────────────────────────────────────────────────
    //  Compilation-unit helpers
    // ──────────────────────────────────────────────────────────────

    /**
     * Creates a FrontendCompilationUnit and registers the given source content under the given name.
     * @param sourceContent  The source text.
     * @param sourceName     A human-readable name.
     * @return A fully initialised compilation unit, or nullptr on failure.
     */
    std::unique_ptr<FrontendCompilationUnit> createUnit(const std::string &sourceContent,
                                                        const std::string &sourceName);

    // ──────────────────────────────────────────────────────────────
    //  Phase runners
    // ──────────────────────────────────────────────────────────────

    /**
     * Runs the tokenization phase on the given unit.
     * @return true if the phase succeeded.
     */
    bool runTokenization(FrontendCompilationUnit *unit);

    /**
     * Runs tokenization + parsing on the given unit.
     * @return true if both phases succeeded.
     */
    bool runThroughParsing(FrontendCompilationUnit *unit);

    /**
     * Runs tokenization + parsing + semantic analysis on the given unit.
     * @return true if all three phases succeeded.
     */
    bool runThroughSemantics(FrontendCompilationUnit *unit);

    /**
     * Runs the full four-phase pipeline on the given unit.
     * @return true if all four phases succeeded.
     */
    bool runFullPipeline(FrontendCompilationUnit *unit);

    // ──────────────────────────────────────────────────────────────
    //  Driver helpers
    // ──────────────────────────────────────────────────────────────

    /**
     * Creates a FrontendCompilerDriver that shares the fixture's error collector and source manager.
     */
    std::unique_ptr<FrontendCompilerDriver> createDriver();

    // ──────────────────────────────────────────────────────────────
    //  File helpers
    // ──────────────────────────────────────────────────────────────

    /**
     * Reads the content of an .ez file from the programs/ subfolder next to this fixture file.
     * @param fileName  File name relative to programs/ (e.g. "full_pipeline_stress.ez").
     * @return The file contents, or an empty string on failure (and an ADD_FAILURE is emitted).
     */
    std::string readProgramFile(const std::string &fileName);

    // ──────────────────────────────────────────────────────────────
    //  Error state helpers
    // ──────────────────────────────────────────────────────────────

    /**
     * Returns true if the shared error collector currently holds at least one fatal error.
     */
    bool hasFatalErrors() const;

  protected:
    /**
     * Returns the path to the programs/ folder, derived from __FILE__.
     */
    static std::filesystem::path getProgramsDir();

  protected:
    std::shared_ptr<SyncLogger> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<ErrorCollector> m_errorCollector;
};

#endif // EZPACKER_FRONTENDCOMPILERTESTFIXTURE_H

