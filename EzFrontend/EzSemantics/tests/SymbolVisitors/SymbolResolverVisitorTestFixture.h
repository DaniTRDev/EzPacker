#ifndef EZPACKER_SYMBOLRESOLVERVISITORTESTFIXTURE_H
#define EZPACKER_SYMBOLRESOLVERVISITORTESTFIXTURE_H

#include "EzSemantics.h"
#include "SymbolVisitors/SymbolAndTypeResolverVisitor.h"
#include <gtest/gtest.h>

class SymbolResolverVisitorTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    /**
     * Tokenizes the input, parses it with the given parser type, and then runs the SymbolAndTypeResolverVisitor.
     * Returns true if all steps succeed.
     *
     * Note: Since SymbolAndTypeResolverVisitor expects symbols to be already defined (by SymbolDefinitionVisitor),
     * you might need to manually define symbols in the context before calling this, or run SymbolDefinitionVisitor
     * first.
     */
    template <typename ParserType> bool runVisitor(const std::string &input, bool runDefinitionVisitor = true)
    {
        m_errorCollector->beginScope();

        // 1. Tokenize
        m_sourceManager->addSourceContent("TEST_SEMANTICS_RESOLVER", input);
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

        // 3. Run Definition Visitor (Optional but usually needed for Resolver)
        if (runDefinitionVisitor)
        {
            m_definitionVisitor = std::make_shared<SymbolDefinitionVisitor>();
            m_definitionVisitor->setSemanticContext(m_semanticContext);

            bool defResult = m_astNode->accept(m_definitionVisitor.get());

            if (!defResult)
            {
                m_errorCollector->endScope(ErrorAction::Propagate);
                return false;
            }
        }

        // 4. Run Resolver Visitor
        m_resolverVisitor = std::make_shared<SymbolAndTypeResolverVisitor>();
        m_resolverVisitor->setSemanticContext(m_semanticContext);

        bool result = m_astNode->accept(m_resolverVisitor.get());

        m_errorCollector->endScope(ErrorAction::Propagate);
        return result;
    }

    std::shared_ptr<BasicSemanticContext> getSemanticContext() { return m_semanticContext; }
    AstNode *getAstNode() { return m_astNode; }

  protected:
    std::shared_ptr<SyncLogger> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<BasicParsingContext> m_parsingContext;
    std::shared_ptr<BasicSemanticContext> m_semanticContext;
    std::shared_ptr<SymbolDefinitionVisitor> m_definitionVisitor;
    std::shared_ptr<SymbolAndTypeResolverVisitor> m_resolverVisitor;
    AstNode *m_astNode;
};

#endif // EZPACKER_SYMBOLRESOLVERVISITORTESTFIXTURE_H
