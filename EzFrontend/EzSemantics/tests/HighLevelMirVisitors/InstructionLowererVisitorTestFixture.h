#ifndef EZPACKER_INSTRUCTIONLOWERERVISITORTESTFIXTURE_H
#define EZPACKER_INSTRUCTIONLOWERERVISITORTESTFIXTURE_H

#include "EzSemantics.h"
#include "HighLevelMirVisitors/ModuleInstructionLowererVisitor.h"
#include "SymbolVisitors/SymbolDefinitionVisitor.h"
#include "SymbolVisitors/SymbolAndTypeResolverVisitor.h"
#include "SymbolVisitors/TypeCheckVisitor.h"
#include "AstNodeParsers/Parsers/InstructionParser.h"
#include "AstNodeParsers/Parsers/LabelParser.h"
#include "AstNodeParsers/Parsers/ModuleParser.h"
#include <gtest/gtest.h>

class InstructionLowererVisitorTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    /**
     * Tokenizes the input, parses it with the given parser type, runs the symbol definition,
     * symbol resolver, and type cast visitors, and finally runs the ModuleInstructionLowererVisitor.
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

        // 3. Run SymbolDefinitionVisitor
        auto symbolDefVisitor = std::make_shared<SymbolDefinitionVisitor>();
        symbolDefVisitor->setSemanticContext(m_semanticContext);

        bool result = true;
        if (auto instr = std::dynamic_pointer_cast<Instruction>(m_astNode))
            result = symbolDefVisitor->visit(instr);
        else if (auto label = std::dynamic_pointer_cast<Label>(m_astNode))
            result = symbolDefVisitor->visit(label);
        else if (auto module = std::dynamic_pointer_cast<Module>(m_astNode))
            result = symbolDefVisitor->visit(module);

        if (!result)
            return false;

        // 4. Run SymbolAndTypeResolverVisitor
        auto symbolResVisitor = std::make_shared<SymbolAndTypeResolverVisitor>();
        symbolResVisitor->setSemanticContext(m_semanticContext);

        if (auto instr = std::dynamic_pointer_cast<Instruction>(m_astNode))
            result = symbolResVisitor->visit(instr);
        else if (auto label = std::dynamic_pointer_cast<Label>(m_astNode))
            result = symbolResVisitor->visit(label);
        else if (auto module = std::dynamic_pointer_cast<Module>(m_astNode))
            result = symbolResVisitor->visit(module);

        if (!result)
            return false;

        // 5. Run TypeCheckVisitor
        auto typeCastVisitor = std::make_shared<TypeCheckVisitor>();
        typeCastVisitor->setSemanticContext(m_semanticContext);

        if (auto instr = std::dynamic_pointer_cast<Instruction>(m_astNode))
            result = typeCastVisitor->visit(instr);
        else if (auto label = std::dynamic_pointer_cast<Label>(m_astNode))
            result = typeCastVisitor->visit(label);
        else if (auto module = std::dynamic_pointer_cast<Module>(m_astNode))
            result = typeCastVisitor->visit(module);

        if (!result)
            return false;

        std::shared_ptr<HighLevelMirModule> block = HighLevelMirModule::create(0, nullptr);

        // 6. Run ModuleInstructionLowererVisitor
        m_visitor = std::make_shared<ModuleInstructionLowererVisitor>(block);
        m_visitor->setSemanticContext(m_semanticContext);

        if (auto instr = std::dynamic_pointer_cast<Instruction>(m_astNode))
            result = m_visitor->visit(instr);
        else if (auto label = std::dynamic_pointer_cast<Label>(m_astNode))
            result = m_visitor->visit(label);
        else if (auto module = std::dynamic_pointer_cast<Module>(m_astNode))
            result = m_visitor->visit(module);

        m_errorCollector->endScope(ErrorAction::Propagate);
        return result;
    }

    std::shared_ptr<BasicSemanticContext> getSemanticContext() { return m_semanticContext; }
    std::shared_ptr<AstNode> getAstNode() { return m_astNode; }
    std::shared_ptr<ModuleInstructionLowererVisitor> getVisitor() { return m_visitor; }

  protected:
    std::shared_ptr<SyncLogger> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<BasicParsingContext> m_parsingContext;
    std::shared_ptr<BasicSemanticContext> m_semanticContext;
    std::shared_ptr<ModuleInstructionLowererVisitor> m_visitor;
    std::shared_ptr<AstNode> m_astNode;
};

#endif // EZPACKER_INSTRUCTIONLOWERERVISITORTESTFIXTURE_H
