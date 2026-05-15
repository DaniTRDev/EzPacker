#include "AstNodeParsers/Parsers/SwitchParser.h"

AstNode *SwitchParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Switch))
    {
        return nullptr; // Silent error not to overlap.
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '(' after 'switch'",
                       "SwitchParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    Variable *switchVar = dynamic_cast<Variable *>(VariableParser().parse(ctx));
    if (!switchVar)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected variable after '(' in switch statement",
                       "SwitchParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ')' after variable in switch statement",
                       "SwitchParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftBrace))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '{' to start switch statement body",
                       "SwitchParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    CodeScope *_default = nullptr;
    TypedPool *nodePool = ctx->getNodePool();
    TypedPoolLinkedList<AstNode> *cases = nodePool->createLinkedList<AstNode>();
    SwitchAstNode *switchAstNode = nodePool->create<SwitchAstNode>(switchVar);

    while (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightBrace))
    {
        if (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Default))
        {
            if (_default)
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Multiple 'default' cases in switch statement body",
                               "SwitchParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }

            if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Colon))
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Expected ':' after 'default' case in switch statement body",
                               "SwitchParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }

            CodeScope *_caseScope = dynamic_cast<CodeScope *>(CodeScopeParser().parse(ctx));
            if (!_caseScope)
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Expected code block after 'default' case in switch statement body",
                               "SwitchParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }

            _default = _caseScope;
            continue;
        }

        if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Case))
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected 'case' or 'default' in switch statement body",
                           "SwitchParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }

        ImmediateOperand *immediate = dynamic_cast<ImmediateOperand *>(ImmediateParser::ImmediateParser().parse(ctx));
        if (!immediate)
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected immediate value after 'case' in switch statement",
                           "SwitchParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }

        if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Colon))
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected ':' after 'case' in switch statement body",
                           "SwitchParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }

        CodeScope *_caseScope = dynamic_cast<CodeScope *>(CodeScopeParser().parse(ctx));
        if (!_caseScope)
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected code block after 'case' in switch statement body",
                           "SwitchParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }

        nodePool->createAndAppendToListBack<SwitchCaseAstNode>(cases, _caseScope, immediate);
    }

    switchAstNode->setCases(cases);
    switchAstNode->setDefault(_default);
    
    return switchAstNode;
}
