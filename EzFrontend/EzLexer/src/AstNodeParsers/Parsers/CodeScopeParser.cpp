#include "AstNodeParsers/Parsers/CodeScopeParser.h"

AstNode *CodeScopeParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftBrace))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '{' at the start of code scope",
                       "CodeScopeParser::CodeScopeParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    TypedPool *nodePool = ctx->getNodePool();
    TypedPoolSlice<AstNode> *expressions = nodePool->createSlice<AstNode>();

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightBrace))
    {
        // At least 1 instruction is expected;
        ParserBatch batch;
        batch.addParsersFromTypeList<LabelParser,
                                     ForParser,
                                     IfParser,
                                     WhileParser,
                                     InstructionParser::InstructionParser,
                                     ContinueParser,
                                     SwitchParser,
                                     BreakParser>();

        AstNode *exprNode = batch.parse(ctx).m_node;
        while (exprNode)
        {
            nodePool->appendToSlice(expressions, exprNode);
            exprNode = batch.parse(ctx).m_node;
        }

        if (expressions->m_numElems == 0)
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected instruction or nested label in module body",
                           "CodeScopeParser::CodeScopeParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }

        if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightBrace))
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected '}' after module body",
                           "CodeScopeParser::CodeScopeParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }
    }

    CodeScope *node = nodePool->create<CodeScope>();
    node->setExpressions(expressions);

    return node;
}
