#ifndef EZPACKER_ASTLOWERERVISITORTESTFIXTURE_H
#define EZPACKER_ASTLOWERERVISITORTESTFIXTURE_H

#include "EzSemantics.h"
#include "AstLowererVisitor/AstLowererVisitor.h"
#include "AstLowererVisitor/LoweringContext.h"
#include "SymbolVisitors/SymbolDefinitionVisitor.h"
#include "SymbolVisitors/SymbolAndTypeResolverVisitor.h"
#include "SymbolVisitors/TypeCheckVisitor.h"
#include <gtest/gtest.h>

/**
 * Test fixture for the entire AST-to-MIR lowering pipeline.
 *
 * The fixture runs:  Tokenize → Parse → SymbolDefinition → SymbolResolver → TypeCheck → AstLowerer
 *
 * After lowering succeeds, helper methods give access to every MIR artefact so tests
 * can verify blocks, instructions, opcodes, operand types/values, symbol-to-MIR linkage,
 * and global data entries.
 */
class AstLowererVisitorTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    // ──────────────────────────────────────────────────────────────
    //  Pipeline runner
    // ──────────────────────────────────────────────────────────────

    /**
     * Runs the entire pipeline (tokenize → parse → semantic passes → lowering) on the
     * given source text using the supplied parser type.
     * Returns true only if every stage succeeds.
     */
    template <typename ParserType> bool runLowering(const std::string &input)
    {
        m_errorCollector->beginScope();

        // 1. Tokenize
        size_t id = m_sourceManager->addSourceContent("TEST_LOWERING", input);
        if (!m_tokenizer->tokenizeBuffer(0, id))
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

        // 3. Symbol Definition
        m_definitionVisitor = std::make_shared<SymbolDefinitionVisitor>();
        m_definitionVisitor->setSemanticContext(m_semanticContext);
        if (!m_astNode->accept(m_definitionVisitor.get()))
        {
            m_errorCollector->endScope(ErrorAction::Propagate);
            return false;
        }

        // 4. Symbol + Type Resolution
        m_resolverVisitor = std::make_shared<SymbolAndTypeResolverVisitor>();
        m_resolverVisitor->setSemanticContext(m_semanticContext);
        if (!m_astNode->accept(m_resolverVisitor.get()))
        {
            m_errorCollector->endScope(ErrorAction::Propagate);
            return false;
        }

        // 5. Type Checking
        m_typeCheckVisitor = std::make_shared<TypeCheckVisitor>();
        m_typeCheckVisitor->setSemanticContext(m_semanticContext);
        if (!m_astNode->accept(m_typeCheckVisitor.get()))
        {
            m_errorCollector->endScope(ErrorAction::Propagate);
            return false;
        }

        // 6. Lowering
        m_loweringCtx =
                std::make_shared<LoweringContext>(m_semanticContext, m_emitter, m_emitterContext, m_globalDataEmitter);
        m_lowererVisitor = std::make_shared<AstLowererVisitor>(m_loweringCtx);
        m_loweringCtx->setOwnerVisitor(m_lowererVisitor.get());

        bool result = m_astNode->accept(m_lowererVisitor.get());

        m_errorCollector->endScope(ErrorAction::Propagate);
        return result;
    }

    // ──────────────────────────────────────────────────────────────
    //  Accessors – AST / Semantic
    // ──────────────────────────────────────────────────────────────

    AstNode *getAstNode() const { return m_astNode; }
    std::shared_ptr<BasicSemanticContext> getSemanticContext() const { return m_semanticContext; }

    // ──────────────────────────────────────────────────────────────
    //  Accessors – MIR Infrastructure
    // ──────────────────────────────────────────────────────────────

    std::shared_ptr<MirEmitterContext> getEmitterContext() const { return m_emitterContext; }
    std::shared_ptr<MirEmitter> getEmitter() const { return m_emitter; }
    std::shared_ptr<MirGlobalDataEmitter> getGlobalDataEmitter() const { return m_globalDataEmitter; }
    std::shared_ptr<LoweringContext> getLoweringContext() const { return m_loweringCtx; }

    // ──────────────────────────────────────────────────────────────
    //  MIR Block helpers
    // ──────────────────────────────────────────────────────────────

    /**
     * Returns all blocks created by the emitter context.
     * Iterates through the block pool.
     */
    TypedPool *getBlockPool() const { return m_emitterContext->getBlockPool(); }

    /**
     * Returns all instructions in a given block.
     */
    TypedPoolSlice<MirInstruction> *getBlockInstructions(MirBlock *block) const { return block->getInstructions(); }

    /**
     * Returns the i-th instruction of a block.
     */
    MirInstruction *getInstruction(MirBlock *block, size_t index) const
    {
        auto instrs = block->getInstructions();
        if (!instrs || index >= instrs->m_numElems)
            return nullptr;
        return instrs->get<MirInstruction>(index);
    }

    /**
     * Returns the number of instructions in a block.
     */
    size_t getInstructionCount(MirBlock *block) const
    {
        auto instrs = block->getInstructions();
        return instrs ? instrs->m_numElems : 0;
    }

    /**
     * Returns the i-th operand of an instruction.
     */
    MirOperand *getOperand(MirInstruction *instr, size_t index) const
    {
        auto ops = instr->getOperands();
        if (!ops || index >= ops->m_numElems)
            return nullptr;
        return ops->get<MirOperand>(index);
    }

    /**
     * Returns the number of operands of an instruction.
     */
    size_t getOperandCount(MirInstruction *instr) const
    {
        auto ops = instr->getOperands();
        return ops ? ops->m_numElems : 0;
    }

    // ──────────────────────────────────────────────────────────────
    //  Symbol-to-MIR linkage helpers
    // ──────────────────────────────────────────────────────────────

    /**
     * Returns the MIR ID linked to a symbol by name.
     * Resolves the symbol first in the given scope (or global scope if nullptr).
     */
    MirId getMirIdForSymbol(const std::string_view &name, Scope *scope = nullptr) const
    {
        Symbol *sym = nullptr;
        if (scope)
            scope->resolve(name, &sym, true);
        else
            m_semanticContext->getGlobalScope()->resolve(name, &sym, true);

        if (!sym)
            return MIRID_INVALID;

        return m_loweringCtx->getMirIdOfSymbol(sym);
    }

    /**
     * Returns true if the given symbol name has been linked to a MIR ID.
     */
    bool isSymbolLinked(const std::string_view &name, Scope *scope = nullptr) const
    {
        return getMirIdForSymbol(name, scope) != MIRID_INVALID;
    }

    // ──────────────────────────────────────────────────────────────
    //  Global data helpers
    // ──────────────────────────────────────────────────────────────

    TypedPool *getDataEntryPool() const { return m_emitterContext->getDataEntryPool(); }

    // ──────────────────────────────────────────────────────────────
    //  Assertion helpers (make tests more concise)
    // ──────────────────────────────────────────────────────────────

    /**
     * Asserts that a block has exactly `expected` instructions, and returns the instruction list.
     */
    TypedPoolSlice<MirInstruction> *expectInstructionCount(MirBlock *block, size_t expected) const
    {
        auto instrs = block->getInstructions();
        EXPECT_NE(instrs, nullptr);
        if (instrs)
            EXPECT_EQ(instrs->m_numElems, expected);
        return instrs;
    }

    /**
     * Asserts that the i-th instruction of a block has the expected opcode.
     */
    MirInstruction *expectOpcode(MirBlock *block, size_t instrIdx, MirInstructionOpCode expected) const
    {
        MirInstruction *instr = getInstruction(block, instrIdx);
        EXPECT_NE(instr, nullptr);
        if (instr)
            EXPECT_EQ(instr->getOpCode(), expected);
        return instr;
    }

    /**
     * Asserts that an instruction has the expected number of operands.
     */
    void expectOperandCount(MirInstruction *instr, size_t expected) const
    {
        ASSERT_NE(instr, nullptr);
        EXPECT_EQ(getOperandCount(instr), expected);
    }

    /**
     * Asserts that the i-th operand of an instruction is of the given type.
     */
    MirOperand *expectOperandType(MirInstruction *instr, size_t opIdx, MirOperandType expected) const
    {
        MirOperand *op = getOperand(instr, opIdx);
        EXPECT_NE(op, nullptr);
        if (op)
            EXPECT_EQ(op->getType(), expected);
        return op;
    }

  protected:
    // Core infrastructure
    std::shared_ptr<SyncLogger> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<SourceLoggingSink> m_sourceSinkLogger;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<BasicParsingContext> m_parsingContext;

    // Semantic infrastructure
    std::shared_ptr<BasicSemanticContext> m_semanticContext;
    std::shared_ptr<SymbolDefinitionVisitor> m_definitionVisitor;
    std::shared_ptr<SymbolAndTypeResolverVisitor> m_resolverVisitor;
    std::shared_ptr<TypeCheckVisitor> m_typeCheckVisitor;

    // MIR infrastructure
    std::shared_ptr<MirEmitterContext> m_emitterContext;
    std::shared_ptr<MirEmitter> m_emitter;
    std::shared_ptr<MirGlobalDataEmitter> m_globalDataEmitter;

    // Lowering infrastructure
    std::shared_ptr<LoweringContext> m_loweringCtx;
    std::shared_ptr<AstLowererVisitor> m_lowererVisitor;

    AstNode *m_astNode;
};

#endif // EZPACKER_ASTLOWERERVISITORTESTFIXTURE_H
