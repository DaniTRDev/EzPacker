#include "CompilationPhases/Tokenization.h"

bool TokenizationPhase::execute(FrontendCompilationUnit *unit)
{
    const std::shared_ptr<SourceManager> &sourceManager = unit->getSourceManager();
    std::shared_ptr<BasicTokenizer> tokenizer =
            std::make_shared<BasicTokenizer>(unit->getErrorCollector(), sourceManager);

    if (!tokenizer->tokenizeBuffer(0, unit->getTargetSourceId()))
    {
        unit->emitError(ErrorSeverity::Fatal, "Tokenization failed", getName());
        return false;
    }

    unit->setTokenizer(tokenizer);
    return true;
}

const char *TokenizationPhase::getName() { return "TokenizationPhase"; }
