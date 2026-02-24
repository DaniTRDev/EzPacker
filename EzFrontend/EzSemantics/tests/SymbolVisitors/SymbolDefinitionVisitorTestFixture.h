#ifndef EZPACKER_SYMBOLDEFINITIONVISITORTESTFIXTURE_H
#define EZPACKER_SYMBOLDEFINITIONVISITORTESTFIXTURE_H

#include "EzSemantics.h"
#include "SymbolVisitors/SymbolDefinitionVisitor.h"
#include <gtest/gtest.h>

class SymbolDefinitionVisitorTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    /**
     * Tokenizes the input, parses it with the given parser type, and then runs the SymbolDefinitionVisitor.
     * Returns true if all steps succeed.
     */
    template <typename ParserType> bool runVisitor(const std::string &input)
    {
        m_errorCollector->beginScope();

        // 1. Tokenize
        m_sourceManager->addSourceContent("TEST_SEMANTICS", input);
        if (!m_tokenizer->tokenizeBuffer((char *)input.data(), 0, input.size()))
            return false;

        // 2. Parse
        m_parsingContext =
                std::make_shared<BasicParsingContext>(m_errorCollector, m_sourceManager, m_tokenizer->getTokens());

        ParserBatch batch;
        batch.addParsersFromTypeList<ParserType>();

        auto parseResult = batch.parse(m_parsingContext);
        m_astNode = parseResult.m_node;

        if (!m_astNode)
        {
            m_errorCollector->endScope(ErrorAction::Propagate);
            return false;
        }

        // 3. Run Visitor
        m_visitor = std::make_shared<SymbolDefinitionVisitor>();
        m_visitor->setSemanticContext(m_semanticContext);

        /**
         * IMPORTANT NOTE: Global variables are not handled because they are essentially a local variable but on top
         * scope.
         */
        bool result = true;

        if (auto instr = std::dynamic_pointer_cast<Instruction>(m_astNode))
        {
            result = m_visitor->visit(instr);
        }
        else if (auto label = std::dynamic_pointer_cast<Label>(m_astNode))
        {
            result = m_visitor->visit(label);
        }
        else if (auto module = std::dynamic_pointer_cast<Module>(m_astNode))
        {
            result = m_visitor->visit(module);
        }

        m_errorCollector->endScope(ErrorAction::Propagate);
        return result;
    }

    std::shared_ptr<BasicSemanticContext> getSemanticContext() { return m_semanticContext; }
    std::shared_ptr<AstNode> getAstNode() { return m_astNode; }

  protected:
    std::shared_ptr<SyncLogger> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<BasicParsingContext> m_parsingContext;
    std::shared_ptr<BasicSemanticContext> m_semanticContext;
    std::shared_ptr<SymbolDefinitionVisitor> m_visitor;
    std::shared_ptr<AstNode> m_astNode;
};

#endif // EZPACKER_SYMBOLDEFINITIONVISITORTESTFIXTURE_H
