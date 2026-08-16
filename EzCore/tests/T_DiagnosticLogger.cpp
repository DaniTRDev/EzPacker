#include "Diagnostics/DiagnosticBuilder.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Diagnostics/DiagnosticScope.h"
#include "SourceManager/SourceManager.h"

#include <gtest/gtest.h>

class DiagTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        m_sourceManager = std::make_unique<SourceManager>(std::filesystem::current_path());
        m_diagLogger = std::make_unique<DiagnosticLogger>(m_sourceManager.get());
        m_diagCollector = std::make_unique<DiagnosticCollector>();

        m_diagCollector->addListener(m_diagLogger.get());

        std::string mockSourceCode = "#include <stdio.h>\n"
                                     "int main() {\n"
                                     "    int message = \"Hello, World!\";\n" // Line index 2 (0-based)
                                     "    return 0;\n"
                                     "}\n";

        // Ingest the string into our source manager under a mock file name
        m_sourceFileId = m_sourceManager->addSourceContent("src/mock_main.c", mockSourceCode);

        if (m_sourceFileId == 0)
        {
            std::cerr << "Critical: Failed to ingest mock source content.\n";
        }
    }

    void TearDown() override {}

  public:
    size_t m_sourceFileId;
    std::unique_ptr<DiagnosticCollector> m_diagCollector;
    std::unique_ptr<DiagnosticLogger> m_diagLogger;
    std::unique_ptr<SourceManager> m_sourceManager;
};

TEST_F(DiagTest, BasicErrorDiagnostic)
{
    // Let's create a primary reference pointing to the assignment expression operator '='
    // Column index 16 (0-based tracking)
    SourceReference assignmentOpRef = m_sourceManager->createReference(16, // col: Starts exactly at the '=' character
                                                                       1,  // length: 1 character wide
                                                                       2,  // line: Index 2 (Line 3)
                                                                       m_sourceFileId // sourceId
    );

    // Let's create a secondary reference pointing to the invalid string token data
    // Column index 18 (0-based tracking)
    SourceReference invalidStringLiteralRef =
            m_sourceManager->createReference(18,            // col: Starts at the opening quote '"'
                                             15,            // length: Length of `"Hello, World!"`
                                             2,             // line: Index 2 (Line 3)
                                             m_sourceFileId // sourceId
            );

    m_diagCollector->beginScope(DiagnosticScopeAction::Commit);
    {
        // Emit the main error utilizing our fluent builder pipeline
        m_diagCollector->builder(Diag_Error, "TypeChecker")
                << &assignmentOpRef << "Incompatible types when assigning to type 'int' from type 'const char*'";

        // InsertAfter a supplemental contextual note highlighting the exact literal mismatch
        m_diagCollector->builder(Diag_Warning, "TypeChecker")
                .appendNote("String literal value cannot be implicitly promoted to scalar integers",
                            &invalidStringLiteralRef);
    }
    // Semicolon completes expression lines, destroying the builders and flushing them.

    // ----------------------------------------------------------------
    // 5. Committing the Pipeline Transaction
    // ----------------------------------------------------------------
    std::cout << "--- Emitting Compiler Diagnostics Transaction ---\n\n";

    // Popping the scope stack validates constraints and triggers the logBridge observer
    m_diagCollector->endScope();
}