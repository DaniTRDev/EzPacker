#include "Diagnostics/DiagnosticBuilder.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Diagnostics/DiagnosticScope.h"
#include "SourceManager/SourceManager.h"

#include <gtest/gtest.h>
#include <memory_resource>

/**
 * Test fixture setting up an arena-backed SourceManager, DiagnosticLogger, and DiagnosticCollector
 * loaded with a mock C source code buffer.
 */
class DiagTest : public ::testing::Test
{
  protected:
    /**
     * Initializes the source manager with mock C code containing a type error on line 3,
     * registers the diagnostic logger subscriber with the collector, and records the file ID.
     */
    void SetUp() override
    {
        m_sourceManager =
                std::make_unique<SourceManager>(std::filesystem::current_path(), std::pmr::get_default_resource());
        m_diagLogger = std::make_unique<DiagnosticLogger>(m_sourceManager.get());
        m_diagCollector = std::make_unique<DiagnosticCollector>();

        m_diagCollector->addListener(m_diagLogger.get());

        // Line 1: "#include <stdio.h>\n" (19 chars: offsets 0-18, \n at 18)
        // Line 2: "int main() {\n"       (13 chars: offsets 19-31, \n at 31)
        // Line 3: "    int message = \"Hello, World!\";\n" (offsets 32-67)
        //         - Line 3 starts at offset 32
        //         - '=' is at offset 32 + 16 = 48
        //         - '"Hello, World!"' starts at offset 32 + 18 = 50
        std::string mockSourceCode = "#include <stdio.h>\n"
                                     "int main() {\n"
                                     "    int message = \"Hello, World!\";\n"
                                     "    return 0;\n"
                                     "}\n";

        m_sourceFileId = m_sourceManager->addSourceContent("src/mock_main.c", mockSourceCode);

        ASSERT_NE(m_sourceFileId, 0u) << "Critical: Failed to ingest mock source content.";
    }

    void TearDown() override {}

  public:
    size_t m_sourceFileId{ 0 };
    std::unique_ptr<DiagnosticCollector> m_diagCollector;
    std::unique_ptr<DiagnosticLogger> m_diagLogger;
    std::unique_ptr<SourceManager> m_sourceManager;
};

/**
 * Verifies that emitting an error diagnostic with attached source span references and
 * supplemental notes correctly formats and logs the diagnostic upon committing the transaction scope.
 */
TEST_F(DiagTest, BasicErrorDiagnostic)
{
    // Absolute start offset in buffer for '='
    size_t assignOpOffset = 48;
    SourceReference *assignmentOpRef = m_sourceManager->createReference(assignOpOffset, // startOffset
                                                                        1,              // length
                                                                        m_sourceFileId  // sourceId
    );

    // Absolute start offset in buffer for `"Hello, World!"`
    size_t strLiteralOffset = 50;
    SourceReference *invalidStringLiteralRef = m_sourceManager->createReference(strLiteralOffset, // startOffset
                                                                                15, // length of `"Hello, World!"`
                                                                                m_sourceFileId // sourceId
    );

    ASSERT_NE(assignmentOpRef, nullptr);
    ASSERT_NE(invalidStringLiteralRef, nullptr);

    m_diagCollector->beginScope(DiagnosticScopeAction::Commit);
    {
        // Emit the main error utilizing our fluent builder pipeline
        m_diagCollector->error("TypeChecker", "Incompatible types when assigning to type 'int' from type 'const char*'")
                << assignmentOpRef;

        // Append a supplemental contextual note highlighting the exact literal mismatch
        m_diagCollector->builder(Diag_Warning, "TypeChecker")
                .appendNote(invalidStringLiteralRef,
                            "String literal value cannot be implicitly promoted to scalar integers");
    }

    std::cout << "--- Emitting Compiler Diagnostics Transaction ---\n\n";

    // Popping the scope stack triggers the listeners
    m_diagCollector->endScope();
    std::cout << std::endl;
}