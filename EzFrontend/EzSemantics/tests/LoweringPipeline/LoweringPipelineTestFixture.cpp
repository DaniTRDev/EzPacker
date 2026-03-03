#include "LoweringPipelineTestFixture.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SetUp / TearDown
// ─────────────────────────────────────────────────────────────────────────────

void LoweringPipelineTestFixture::SetUp()
{
    m_logger = EzLogger::createSyncLogger("LOWERING_PIPELINE");
    m_sourceManager = std::make_shared<SourceManager>();
    m_sourceSinkLogger = std::make_shared<SourceLoggingSink>(m_logger.get());
    m_errorCollector = std::make_shared<ErrorCollector>();
    m_semanticContext = std::make_shared<BasicSemanticContext>(m_errorCollector, m_sourceManager);

    // MIR
    m_emitterContext = std::make_shared<MirEmitterContext>(m_errorCollector, m_sourceManager);
    m_emitter = std::make_shared<MirEmitter>(m_emitterContext.get());
    m_globalDataEmitter = std::make_shared<MirGlobalDataEmitter>(m_emitterContext.get());

    m_astNode = nullptr;

    m_errorCollector->addSubscriber(
            [](void *userParam, const std::shared_ptr<Error> &error) -> void
            {
                auto *fixture = static_cast<LoweringPipelineTestFixture *>(userParam);
                if (error->m_sourceRef.m_valid)
                {
                    g_logger->pushLog(LogMessage(
                            "[{}]{} {}:{}:{} {} \n\t {}",
                            error->m_sender,
                            error->m_timeStamp,
                            fixture->m_sourceManager->getSourceName(error->m_sourceRef.m_sourceFileId),
                            error->m_sourceRef.m_line,
                            error->m_sourceRef.m_col,
                            error->m_message,
                            fixture->m_sourceManager->getReferenceContent(error->m_sourceRef)));
                }
                else
                {
                    g_logger->pushLog(
                            LogMessage("[{}]{} {}", error->m_sender, error->m_timeStamp, error->m_message));
                }
            },
            this);

    Test::SetUp();
}

void LoweringPipelineTestFixture::TearDown()
{
    m_lowererVisitor.reset();
    m_loweringCtx.reset();
    m_globalDataEmitter.reset();
    m_emitter.reset();
    m_emitterContext.reset();
    m_typeCheckVisitor.reset();
    m_resolverVisitor.reset();
    m_definitionVisitor.reset();
    m_semanticContext.reset();
    m_parsingContext.reset();
    m_tokenizer.reset();
    m_sourceSinkLogger.reset();
    m_sourceManager.reset();
    m_logger.reset();

    Test::TearDown();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pipeline runners
// ─────────────────────────────────────────────────────────────────────────────

std::filesystem::path LoweringPipelineTestFixture::getProgramsDir()
{
    // __FILE__ resolves to .../tests/LoweringPipeline/LoweringPipelineTestFixture.cpp
    std::filesystem::path thisFile(__FILE__);
    return thisFile.parent_path() / "programs";
}

bool LoweringPipelineTestFixture::runFromFile(const std::string &fileName)
{
    auto path = getProgramsDir() / fileName;
    std::ifstream ifs(path, std::ios::in);
    if (!ifs.is_open())
    {
        ADD_FAILURE() << "Could not open test program: " << path.string();
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    return runPipeline(content, fileName);
}

bool LoweringPipelineTestFixture::runFromSource(const std::string &input)
{
    return runPipeline(input, "INLINE_SOURCE");
}

bool LoweringPipelineTestFixture::runPipeline(const std::string &input, const std::string &sourceName)
{
    m_errorCollector->beginScope();

    // 1. Tokenize
    m_tokenizer = std::make_shared<BasicTokenizer>(m_errorCollector, m_sourceManager, sourceName);
    m_sourceManager->addSourceContent(sourceName, input);
    if (!m_tokenizer->tokenizeBuffer((char *)input.data(), 0, input.size()))
    {
        m_errorCollector->endScope(ErrorAction::Propagate);
        return false;
    }

    // 2. Parse
    m_parsingContext =
            std::make_shared<BasicParsingContext>(m_errorCollector, m_sourceManager, m_tokenizer->getTokens());

    ParserBatch batch;
    batch.addParsersFromTypeList<ModuleParser::ModuleParser>();

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

// ─────────────────────────────────────────────────────────────────────────────
//  AST / Semantic accessors
// ─────────────────────────────────────────────────────────────────────────────

AstNode *LoweringPipelineTestFixture::getAstNode() const { return m_astNode; }

Module *LoweringPipelineTestFixture::getModule() const { return dynamic_cast<Module *>(m_astNode); }

std::shared_ptr<BasicSemanticContext> LoweringPipelineTestFixture::getSemanticContext() const
{
    return m_semanticContext;
}

Symbol *LoweringPipelineTestFixture::resolveInModuleScope(const std::string_view &name) const
{
    auto *mod = getModule();
    if (!mod)
        return nullptr;
    auto *scopedAnnot = mod->getAnnotation<ScopedSymbolAnnotation>();
    if (!scopedAnnot)
        return nullptr;
    Scope *scope = scopedAnnot->getOwnedScope();
    if (!scope)
        return nullptr;

    Symbol *sym = nullptr;
    scope->resolve(name, &sym, true);
    return sym;
}

Symbol *LoweringPipelineTestFixture::resolveInLabelScope(const std::string_view &labelName,
                                                          const std::string_view &symbolName) const
{
    auto *mod = getModule();
    if (!mod || !mod->getBody())
        return nullptr;

    auto *bodyExprs = mod->getBody()->getExpressions();
    if (!bodyExprs)
        return nullptr;

    for (size_t i = 0; i < bodyExprs->m_numElems; ++i)
    {
        auto *node = bodyExprs->get<AstNode>(i);
        if (!node || node->getType() != AstNodeType::Label)
            continue;
        auto *label = dynamic_cast<Label *>(node);
        if (!label || label->getLabelName() != labelName)
            continue;

        auto *scopedAnnot = label->getAnnotation<ScopedSymbolAnnotation>();
        if (!scopedAnnot)
            return nullptr;
        Scope *scope = scopedAnnot->getOwnedScope();
        if (!scope)
            return nullptr;

        Symbol *sym = nullptr;
        scope->resolve(symbolName, &sym, false);
        return sym;
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  MIR accessors
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<MirEmitterContext> LoweringPipelineTestFixture::getEmitterContext() const { return m_emitterContext; }
std::shared_ptr<MirEmitter> LoweringPipelineTestFixture::getEmitter() const { return m_emitter; }
std::shared_ptr<MirGlobalDataEmitter> LoweringPipelineTestFixture::getGlobalDataEmitter() const
{
    return m_globalDataEmitter;
}
std::shared_ptr<LoweringContext> LoweringPipelineTestFixture::getLoweringContext() const { return m_loweringCtx; }

// ─────────────────────────────────────────────────────────────────────────────
//  Block / Instruction / Operand helpers
// ─────────────────────────────────────────────────────────────────────────────

MirInstruction *LoweringPipelineTestFixture::getInstruction(MirBlock *block, size_t index) const
{
    if (!block)
        return nullptr;
    auto *instrs = block->getInstructions();
    if (!instrs || index >= instrs->m_numElems)
        return nullptr;
    return instrs->get<MirInstruction>(index);
}

size_t LoweringPipelineTestFixture::getInstructionCount(MirBlock *block) const
{
    if (!block)
        return 0;
    auto *instrs = block->getInstructions();
    return instrs ? instrs->m_numElems : 0;
}

MirOperand *LoweringPipelineTestFixture::getOperand(MirInstruction *instr, size_t index) const
{
    if (!instr)
        return nullptr;
    auto *ops = instr->getOperands();
    if (!ops || index >= ops->m_numElems)
        return nullptr;
    return ops->get<MirOperand>(index);
}

size_t LoweringPipelineTestFixture::getOperandCount(MirInstruction *instr) const
{
    if (!instr)
        return 0;
    auto *ops = instr->getOperands();
    return ops ? ops->m_numElems : 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Symbol-to-MIR linkage
// ─────────────────────────────────────────────────────────────────────────────

bool LoweringPipelineTestFixture::isSymbolLinked(Symbol *sym) const
{
    return m_semanticContext->isSymbolLinkedToMir(sym);
}

MirId LoweringPipelineTestFixture::getMirId(Symbol *sym) const
{
    return m_semanticContext->getMirIdOfSymbol(sym);
}

MirId LoweringPipelineTestFixture::expectSymbolLinked(const std::string_view &name) const
{
    Symbol *sym = resolveInModuleScope(name);
    EXPECT_NE(sym, nullptr) << "Symbol '" << name << "' not found in module scope";
    if (!sym)
        return MIRID_INVALID;
    EXPECT_TRUE(isSymbolLinked(sym)) << "Symbol '" << name << "' not linked to MIR";
    MirId id = getMirId(sym);
    EXPECT_NE(id, MIRID_INVALID);
    return id;
}

void LoweringPipelineTestFixture::expectDistinctMirIds(const std::string_view &nameA,
                                                        const std::string_view &nameB) const
{
    MirId idA = expectSymbolLinked(nameA);
    MirId idB = expectSymbolLinked(nameB);
    EXPECT_NE(idA, idB) << "Symbols '" << nameA << "' and '" << nameB << "' should have distinct MIR IDs";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Instruction assertion helpers
// ─────────────────────────────────────────────────────────────────────────────

MirInstruction *LoweringPipelineTestFixture::expectOpcode(MirBlock *block, size_t instrIdx,
                                                           MirInstructionOpCode expected) const
{
    MirInstruction *instr = getInstruction(block, instrIdx);
    EXPECT_NE(instr, nullptr);
    if (instr)
        EXPECT_EQ(instr->getOpCode(), expected);
    return instr;
}

void LoweringPipelineTestFixture::expectOperandCount(MirInstruction *instr, size_t expected) const
{
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(getOperandCount(instr), expected);
}

MirOperand *LoweringPipelineTestFixture::expectOperandType(MirInstruction *instr, size_t opIdx,
                                                            MirOperandType expected) const
{
    MirOperand *op = getOperand(instr, opIdx);
    EXPECT_NE(op, nullptr);
    if (op)
        EXPECT_EQ(op->getType(), expected);
    return op;
}

TypedPoolSlice<MirInstruction> *LoweringPipelineTestFixture::expectInstructionCount(MirBlock *block,
                                                                                     size_t expected) const
{
    auto *instrs = block->getInstructions();
    EXPECT_NE(instrs, nullptr);
    if (instrs)
        EXPECT_EQ(instrs->m_numElems, expected);
    return instrs;
}

