#ifndef EZPACKER_INSTRUCTIONSEMANTICCHECKERVISITORTESTFIXTURE_H
#define EZPACKER_INSTRUCTIONSEMANTICCHECKERVISITORTESTFIXTURE_H

#include "EzSemantics.h"
#include "HighLevelMirVisitors/InstructionSemanticCheckerVisitor.h"
#include "HighLevelMir/HighLevelMirInstruction.h"
#include "HighLevelMir/HighLevelMirBlock.h"
#include "HighLevelMir/HighLevelMirModule.h"
#include <gtest/gtest.h>

class InstructionSemanticCheckerVisitorTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    /**
     * Runs the visitor on a manually constructed MIR block.
     */
    bool runVisitor(std::shared_ptr<HighLevelMirBlock> block)
    {
        m_errorCollector->beginScope();

        m_visitor = std::make_shared<InstructionSemanticCheckerVisitor>();
        m_visitor->setSemanticContext(m_semanticContext);

        bool result = m_visitor->visit(*block);

        m_errorCollector->endScope(ErrorAction::Propagate);
        return result;
    }

    /**
     * Helper to create a dummy source reference.
     */
    std::shared_ptr<SourceReference> createDummySourceRef()
    {
        return std::make_shared<SourceReference>(
                SourceReference{ .m_col = 0, .m_length = 0, .m_line = 0, .m_sourceFile = "test_file.ez" });
    }

    std::shared_ptr<BasicSemanticContext> getSemanticContext() { return m_semanticContext; }

  protected:
    std::shared_ptr<SyncLogger> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<BasicSemanticContext> m_semanticContext;
    std::shared_ptr<InstructionSemanticCheckerVisitor> m_visitor;
};

#endif // EZPACKER_INSTRUCTIONSEMANTICCHECKERVISITORTESTFIXTURE_H
