#include "AstNodeParsers/LabelParser.h"

std::shared_ptr<Label> LabelParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<Label> node;
    std::string name;
    std::vector<LabelExpression> expressions;
    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier, name, ctx, LogMessage("Invalid label name"));
    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Colon, ctx, LogMessage("Expected ':' after label name"));
    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftBrace, ctx, LogMessage("Expected '{{' after label ':'"));

    /*
     * A label may or may not have any instructions.
     */
    if (!AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightBrace, ctx))
    {
        // At least 1 instruction is expected before the right brace ('}')
        if (!AstNodeParsingUtils::expectType(_TokenType::Identifier, ctx))
        {
            return nullptr;
        }

        while (AstNodeParsingUtils::expectType(_TokenType::Identifier, ctx))
        {
            std::shared_ptr<AstNode> exprNode;
            LabelExpression expr;

            if (exprNode = AstNodeParsingUtils::tryParsers<InstructionParser, LabelParser>(ctx); !exprNode)
            {
                return nullptr;
            }

            if (exprNode->getType() == AstNodeType::Instruction)
                expr = std::move(std::dynamic_pointer_cast<Instruction>(exprNode));
            else if (exprNode->getType() == AstNodeType::Label)
                expr = std::move(std::dynamic_pointer_cast<Label>(exprNode));
            else
            {
                return nullptr;
            }

            expressions.push_back(std::move(expr));
        }

        AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightBrace,
                                                  ctx,
                                                  LogMessage("Expected '}}' after label body"));
    }

    node = std::make_shared<Label>(std::move(name), std::move(expressions));
    return std::move(node);
}
