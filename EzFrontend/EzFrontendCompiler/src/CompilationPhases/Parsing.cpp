#include "CompilationPhases/Parsing.h"

bool ParsingPhase::execute(struct FrontendCompilationUnit *unit)
{
    const std::shared_ptr<BasicTokenizer> &tokenizer = unit->getTokenizer();
    const std::shared_ptr<ErrorCollector> &errorCollector = unit->getErrorCollector();
    const std::shared_ptr<SourceManager> &sourceManager = unit->getSourceManager();

    ParserBatch batch;
    batch.addParsersFromTypeList<IncludeParser, VariableParser, ModuleParser::ModuleParser>();

    std::shared_ptr<BasicParsingContext> parsingContext =
            std::make_shared<BasicParsingContext>(errorCollector, sourceManager, tokenizer->getTokens());

    TypedPool *nodePool = parsingContext->getNodePool();
    TypedPoolSlice<AstNode> *globalScopeAstNodes = nodePool->createSlice<AstNode>();

    while (parsingContext->canPeek())
    {
        AstNode *node = batch.parse(parsingContext).m_node;

        if (!node)
        {
            // There must be at least 1 fatal error that made the compilation stop.
            break;
        }

        nodePool->appendToSlice(globalScopeAstNodes, node);
    }

    if (errorCollector->doesCurrentScopeHasFatalErrors())
    {
        unit->emitError(ErrorSeverity::Fatal, "Parsing failed due to previous errors", getName());
        return false;
    }

    unit->setParsingContext(parsingContext);
    unit->setGlobalScopeAstNodes(globalScopeAstNodes);
    return true;
}

const char *ParsingPhase::getName() { return "ParsingPhase"; }
