#ifndef EZPACKER_TYPECASTVISITORTESTFIXTURE_H
#define EZPACKER_TYPECASTVISITORTESTFIXTURE_H

#include "EzSemantics.h"
#include "SymbolVisitors/TypeCheckVisitor.h"
#include <gtest/gtest.h>

class TypeCastVisitorTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    /**
     * Tokenizes the input, parses it with the given parser type, and then runs the TypeCheckVisitor.
     * Returns true if all steps succeed.
     * 
     * Note: Since TypeCheckVisitor expects symbols to be already defined and resolved,
     * we need to run Definition and Resolver visitors first.
     */
    template <typename ParserType> bool runVisitor(const std::string &input, bool runDefinitionAndResolver = true)
    {
        m_errorCollector->beginScope();

        // 1. Tokenize
        m_sourceManager->addSourceContent("TEST_SEMANTICS_TYPECAST", input);
        if (!m_tokenizer->tokenizeBuffer((char *)input.data(), 0, input.size()))
        {
            m_errorCollector->endScope(ErrorAction::Propagate);
            return false;
        }

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

        // 3. Run Definition and Resolver Visitors
        if (runDefinitionAndResolver)
        {
            m_definitionVisitor = std::make_shared<SymbolDefinitionVisitor>();
            m_definitionVisitor->setSemanticContext(m_semanticContext);
            
            bool defResult = false;
            if (auto instr = std::dynamic_pointer_cast<Instruction>(m_astNode))
                defResult = m_definitionVisitor->visit(instr);
            else if (auto label = std::dynamic_pointer_cast<Label>(m_astNode))
                defResult = m_definitionVisitor->visit(label);
            else if (auto module = std::dynamic_pointer_cast<Module>(m_astNode))
                defResult = m_definitionVisitor->visit(module);
            
            if (!defResult) 
            {
                m_errorCollector->endScope(ErrorAction::Propagate);
                return false;
            }

            m_resolverVisitor = std::make_shared<SymbolAndTypeResolverVisitor>();
            m_resolverVisitor->setSemanticContext(m_semanticContext);

            bool resResult = false;
            if (auto instr = std::dynamic_pointer_cast<Instruction>(m_astNode))
                resResult = m_resolverVisitor->visit(instr);
            else if (auto label = std::dynamic_pointer_cast<Label>(m_astNode))
                resResult = m_resolverVisitor->visit(label);
            else if (auto module = std::dynamic_pointer_cast<Module>(m_astNode))
                resResult = m_resolverVisitor->visit(module);

            if (!resResult) 
            {
                m_errorCollector->endScope(ErrorAction::Propagate);
                return false;
            }
        }

        // 4. Run TypeCast Visitor
        m_typeCastVisitor = std::make_shared<TypeCheckVisitor>();
        m_typeCastVisitor->setSemanticContext(m_semanticContext);
        
        bool result = false;
        if (auto instr = std::dynamic_pointer_cast<Instruction>(m_astNode))
        {
            result = m_typeCastVisitor->visit(instr);
        }
        else if (auto label = std::dynamic_pointer_cast<Label>(m_astNode))
        {
            result = m_typeCastVisitor->visit(label);
        }
        else if (auto module = std::dynamic_pointer_cast<Module>(m_astNode))
        {
            result = m_typeCastVisitor->visit(module);
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
    std::shared_ptr<SymbolDefinitionVisitor> m_definitionVisitor;
    std::shared_ptr<SymbolAndTypeResolverVisitor> m_resolverVisitor;
    std::shared_ptr<TypeCheckVisitor> m_typeCastVisitor;
    std::shared_ptr<AstNode> m_astNode;
};

#endif // EZPACKER_TYPECASTVISITORTESTFIXTURE_H
