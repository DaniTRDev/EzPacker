#include "AstNodeParsers/ModuleParser.h"

std::shared_ptr<Module> ModuleParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    // Order is important, we can't alphabetically order these variables.
    std::shared_ptr<::ModuleHeader> header = ModuleHeaderParser().parse(ctx);
    if (!header)
    {
        return nullptr;
    }

    std::shared_ptr<::ModuleBody> body = ModuleBodyParser().parse(ctx);
    if (!body)
    {
        return nullptr;
    }

    return std::make_shared<Module>(std::move(body), std::move(header));
}

std::shared_ptr<ModuleBody> ModuleParser::ModuleBodyParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<ModuleBody> node;
    std::vector<ModuleBodyExpr> expressions;

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftBrace,
                                              ctx,
                                              LogMessage("Expected '{{' at the start of module body"));

    if (!AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightBrace, ctx))
    {
        // At least 1 instruction is expected;
        do
        {
            std::shared_ptr<AstNode> exprNode;
            ModuleBodyExpr expr;

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

        } while (!AstNodeParsingUtils::expectType(_TokenType::RightBrace, ctx));

        AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightBrace,
                                                  ctx,
                                                  LogMessage("Expected '}}' at the end of module body"));
    }

    node = std::make_shared<ModuleBody>(std::move(expressions));
    return std::move(node);
}

std::shared_ptr<ModuleHeader> ModuleParser::ModuleHeaderParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<::ModuleHeader> node;
    std::string name, returnType;
    std::vector<std::shared_ptr<AstNode>> arguments;

    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier,
                                                  returnType,
                                                  ctx,
                                                  LogMessage("Invalid module return type"));
    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier, name, ctx, LogMessage("Invalid module name"));
    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftParen,
                                              ctx,
                                              LogMessage("Expected '(' at the start of module argument definition"));

    if (!AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightParen, ctx))
    {
        // At least 1 argument is expected.
        do
        {
            std::shared_ptr<AstNode> argument;
            if (argument = AstNodeParsingUtils::tryParsers<ImmediateOperandParser, VariableParser>(ctx); !argument)
            {
                return nullptr;
            }

            arguments.push_back(std::move(argument));
        } while (AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Comma, ctx));

        AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightParen,
                                                  ctx,
                                                  LogMessage("Expected ')' at the end of module argument definition"));
    }

    node = std::make_shared<::ModuleHeader>(std::move(name), std::move(returnType), std::move(arguments));
    return std::move(node);
}
