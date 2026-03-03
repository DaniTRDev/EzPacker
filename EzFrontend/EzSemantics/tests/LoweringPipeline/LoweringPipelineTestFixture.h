#ifndef EZPACKER_LOWERINGPIPELINETESTFIXTURE_H
#define EZPACKER_LOWERINGPIPELINETESTFIXTURE_H

#include "EzSemantics.h"
#include "AstLowererVisitor/AstLowererVisitor.h"
#include "AstLowererVisitor/LoweringContext.h"
#include "SymbolVisitors/SymbolDefinitionVisitor.h"
#include "SymbolVisitors/SymbolAndTypeResolverVisitor.h"
#include "SymbolVisitors/TypeCheckVisitor.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

/**
 * Test fixture that runs the entire pipeline (tokenize → parse → semantic passes → lowering)
 * from .ez source files on disk, then verifies the resulting MIR.
 */
class LoweringPipelineTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    // ──────────────────────────────────────────────────────────────
    //  Pipeline runners
    // ──────────────────────────────────────────────────────────────

    /**
     * Runs the full pipeline on an .ez file located in the programs/ subfolder.
     * @param fileName  File name relative to the programs/ folder (e.g. "arithmetic.ez").
     * @return true if all phases succeeded.
     */
    bool runFromFile(const std::string &fileName);

    /**
     * Runs the full pipeline on an inline source string using ModuleParser.
     * @param input  Source code string.
     * @return true if all phases succeeded.
     */
    bool runFromSource(const std::string &input);

    // ──────────────────────────────────────────────────────────────
    //  AST / Semantic accessors
    // ──────────────────────────────────────────────────────────────

    AstNode *getAstNode() const;
    Module *getModule() const;
    std::shared_ptr<BasicSemanticContext> getSemanticContext() const;

    /**
     * Resolves a symbol by name in the module's own scope. Returns nullptr if not found.
     */
    Symbol *resolveInModuleScope(const std::string_view &name) const;

    /**
     * Resolves a symbol by name in a label's scope. The label must be a child of the module body.
     */
    Symbol *resolveInLabelScope(const std::string_view &labelName, const std::string_view &symbolName) const;

    // ──────────────────────────────────────────────────────────────
    //  MIR accessors
    // ──────────────────────────────────────────────────────────────

    std::shared_ptr<MirEmitterContext> getEmitterContext() const;
    std::shared_ptr<MirEmitter> getEmitter() const;
    std::shared_ptr<MirGlobalDataEmitter> getGlobalDataEmitter() const;
    std::shared_ptr<LoweringContext> getLoweringContext() const;

    // ──────────────────────────────────────────────────────────────
    //  MIR Block / Instruction / Operand helpers
    // ──────────────────────────────────────────────────────────────

    MirInstruction *getInstruction(MirBlock *block, size_t index) const;
    size_t getInstructionCount(MirBlock *block) const;
    MirOperand *getOperand(MirInstruction *instr, size_t index) const;
    size_t getOperandCount(MirInstruction *instr) const;

    // ──────────────────────────────────────────────────────────────
    //  Symbol-to-MIR linkage helpers
    // ──────────────────────────────────────────────────────────────

    bool isSymbolLinked(Symbol *sym) const;
    MirId getMirId(Symbol *sym) const;

    /**
     * Asserts that the named symbol in module scope is linked to a unique MIR ID.
     */
    MirId expectSymbolLinked(const std::string_view &name) const;

    /**
     * Asserts that two named symbols in module scope are linked to DIFFERENT MIR IDs.
     */
    void expectDistinctMirIds(const std::string_view &nameA, const std::string_view &nameB) const;

    // ──────────────────────────────────────────────────────────────
    //  Instruction assertion helpers
    // ──────────────────────────────────────────────────────────────

    MirInstruction *expectOpcode(MirBlock *block, size_t instrIdx, MirInstructionOpCode expected) const;
    void expectOperandCount(MirInstruction *instr, size_t expected) const;
    MirOperand *expectOperandType(MirInstruction *instr, size_t opIdx, MirOperandType expected) const;
    TypedPoolSlice<MirInstruction> *expectInstructionCount(MirBlock *block, size_t expected) const;

  private:
    /**
     * Internal: runs all phases on the already-set source content.
     */
    bool runPipeline(const std::string &input, const std::string &sourceName);

    /**
     * Returns the path to the programs/ folder, derived from __FILE__.
     */
    static std::filesystem::path getProgramsDir();

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

#endif // EZPACKER_LOWERINGPIPELINETESTFIXTURE_H

