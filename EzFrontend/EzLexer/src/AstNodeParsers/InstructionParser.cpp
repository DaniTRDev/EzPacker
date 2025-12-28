#include "AstNodeParsers/InstructionParser.h"

std::shared_ptr<Instruction> InstructionParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    return std::dynamic_pointer_cast<Instruction>(
            AstNodeParsingUtils::tryParsers<InstructionParser::CallInstructionParser, RegularInstruction>(
                    ctx,
                    LogMessage("Invalid instruction")));
}

std::shared_ptr<::CallInstruction>
InstructionParser::CallInstructionParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<::CallInstruction> node;
    std::string instructionName, calleeName, returnType;
    std::vector<std::shared_ptr<AstNode>> arguments;

    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier,
                                                  instructionName,
                                                  ctx,
                                                  LogMessage("Invalid instruction name"));
    std::ranges::transform(instructionName,
                           instructionName.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (instructionName != "call")
    {
        return nullptr;
    }

    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier,
                                                  returnType,
                                                  ctx,
                                                  LogMessage("Invalid callee function return type"));
    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier,
                                                  calleeName,
                                                  ctx,
                                                  LogMessage("Invalid callee function name"));
    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftParen,
                                              ctx,
                                              LogMessage("Expected '(' before function call arguments"));

    if (!AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightParen, ctx))
    {
        // We expect call parameters.
        do
        {
            std::shared_ptr<AstNode> argument;
            if (argument = AstNodeParsingUtils::tryParsers<ImmediateOperandParser, VariableParser>(
                        ctx,
                        LogMessage("Invalid argument for function call"));
                !argument)
            {
                return nullptr;
            }
            arguments.push_back(std::move(argument));
        } while (AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Comma, ctx));

        AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightParen,
                                                  ctx,
                                                  LogMessage("Expected ')' after function call arguments"));
    }
    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::SemiColon, ctx, LogMessage("Expected ';' after instruction"));

    node = std::make_shared<::CallInstruction>(std::move(calleeName), std::move(returnType), std::move(arguments));
    return std::move(node);
}

std::shared_ptr<Instruction> InstructionParser::RegularInstruction::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<Instruction> node;
    std::string instructionName;
    std::vector<std::shared_ptr<AstNode>> operands;

    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier,
                                                  instructionName,
                                                  ctx,
                                                  LogMessage("Invalid instruction name"));
    std::ranges::transform(instructionName,
                           instructionName.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (!AstNodeParsingUtils::expectTypeAndConsume(_TokenType::SemiColon, ctx))
    {
        // At least 1 operand is expected before the semicolon (';')
        do
        {
            std::shared_ptr<AstNode> operand;
            if (operand = AstNodeParsingUtils::tryParsers<ImmediateOperandParser, VariableParser, MemoryOperandParser>(
                        ctx,
                        LogMessage("Invalid operand for instruction"));
                !operand)
            {
                return nullptr;
            }
            operands.push_back(std::move(operand));
        } while (AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Comma, ctx));

        AstNodeParsingUtils::expectTypeAndConsume(_TokenType::SemiColon,
                                                  ctx,
                                                  LogMessage("Expected ';' after instruction"));
    }

    node = std::make_shared<Instruction>(std::move(instructionName), std::move(operands));
    return std::move(node);
}
