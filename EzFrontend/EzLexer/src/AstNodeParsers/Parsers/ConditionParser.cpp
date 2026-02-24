#include "AstNodeParsers/Parsers/ConditionParser.h"

std::shared_ptr<AstNode> ConditionParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    ConditionComparisonType comparisonType;
    std::shared_ptr<ConditionAstNode> node;
    std::shared_ptr<AstNode> leftOperand;
    std::shared_ptr<AstNode> rightOperand;

    if (leftOperand = VariableParser().parse(ctx); !leftOperand)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected variable as left operand of condition",
                       "ConditionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    TokenInformation comparisonToken;
    if (!ctx->consumeIf(ParsingCondition::TokenType, &comparisonToken, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected comparison operator (e.g., 'EQ', 'NE', 'GT', 'LT', 'GE', 'LE')",
                       "ConditionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    ParserBatch batch;
    batch.addParsersFromTypeList<ImmediateParser::Integer, ImmediateParser::Float, VariableParser>();

    if (rightOperand = batch.parse(ctx).m_node; !rightOperand)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected variable, integer or float as the right operand of condition",
                       "ConditionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (comparisonToken.m_str == "EQ")
    {
        comparisonType = ConditionComparisonType::Equal;
    }
    else if (comparisonToken.m_str == "NE")
    {
        comparisonType = ConditionComparisonType::NotEqual;
    }
    else if (comparisonToken.m_str == "GT")
    {
        comparisonType = ConditionComparisonType::GreaterThan;
    }
    else if (comparisonToken.m_str == "LT")
    {
        comparisonType = ConditionComparisonType::LessThan;
    }
    else if (comparisonToken.m_str == "GE")
    {
        comparisonType = ConditionComparisonType::GreaterThanOrEqual;
    }
    else if (comparisonToken.m_str == "LE")
    {
        comparisonType = ConditionComparisonType::LessThanOrEqual;
    }
    else
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Invalid comparison operator: " + comparisonToken.m_str,
                       "ConditionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    node = std::make_shared<ConditionAstNode>(comparisonType, leftOperand, rightOperand);
    return node;
}
