#include "AstNodeParsers/Parsers/InstructionParser.h"

namespace InstructionParser
{

ParserBatch CreateInstructionOperandBatch()
{
    ParserBatch batch;
    batch.addParsersFromTypeList<ImmediateParser::ImmediateParser,
                                 VariableParser,
                                 MemoryOperandParser::MemoryOperandParser>();

    return batch;
}

std::shared_ptr<AstNode> CallInstructionParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation instructionToken, calleeNameToken, returnTypeToken;
    std::shared_ptr<CallInstruction> node;
    std::vector<std::shared_ptr<AstNode>> arguments;

    if (!ctx->consumeIf(ParsingCondition::TokenType, &instructionToken, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected identifier for instruction mnemonic",
                       "InstructionParser::CallInstructionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::string &instructionName = instructionToken.m_str;
    std::ranges::transform(instructionName,
                           instructionName.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (instructionName != "call")
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Given instruction mnemonic is not CALL",
                       "InstructionParser::CallInstructionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    /**
     * If this place is reached, the expression is somewhat:
     * identifier(call)
     * Which can only be a CallInstruction.
     */

    if (!ctx->consumeIf(ParsingCondition::TokenType, &returnTypeToken, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Invalid callee function return type",
                       "InstructionParser::CallInstructionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, &calleeNameToken, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Invalid callee function name",
                       "InstructionParser::CallInstructionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '(' before function call arguments",
                       "InstructionParser::CallInstructionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::string &calleeName = calleeNameToken.m_str, &returnType = returnTypeToken.m_str;

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {
        // We expect call parameters.
        ParserBatch batch = CreateInstructionOperandBatch();
        do
        {
            std::shared_ptr<AstNode> argument = batch.parse(ctx).m_node;

            if (!argument)
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Invalid expression in call argument",
                               "InstructionParser::CallInstructionParser",
                               ctx->getLastSourceReference());

                return nullptr;
            }

            arguments.push_back(std::move(argument));
        } while (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma));

        if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected ')' after function call arguments",
                           "InstructionParser::CallInstructionParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::SemiColon))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ';' after instruction",
                       "InstructionParser::CallInstructionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    node = std::make_shared<::CallInstruction>(std::move(calleeName), std::move(returnType), std::move(arguments));
    return std::move(node);
}

std::shared_ptr<AstNode> NonCallInstructionParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation token;
    std::shared_ptr<Instruction> node;
    std::vector<std::shared_ptr<AstNode>> operands;

    if (!ctx->consumeIf(ParsingCondition::TokenType, &token, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected identifier for instruction mnemonic",
                       "InstructionParser::NonCallInstructionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::string &instructionName = token.m_str;
    std::ranges::transform(instructionName,
                           instructionName.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::SemiColon))
    {
        // At least 1 operand is expected before the semicolon (';')
        static ParserBatch batch = CreateInstructionOperandBatch();
        do
        {
            std::shared_ptr<AstNode> operand = batch.parse(ctx).m_node;

            if (!operand)
            {
                ctx->emitError(ErrorSeverity::Soft,
                               "Invalid expression on instruction operand",
                               "InstructionParser::NonCallInstructionParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }

            operands.push_back(std::move(operand));
        } while (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma));

        // If this place have been reached, this expression can only be an instruction.
        if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::SemiColon))
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected ';' after instruction operands",
                           "InstructionParser::NonCallInstructionParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }
    }

    node = std::make_shared<Instruction>(std::move(instructionName), std::move(operands));
    return std::move(node);
}

std::shared_ptr<AstNode> InstructionParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    ParserBatch batch;
    batch.addParsersFromTypeList<CallInstructionParser, NonCallInstructionParser>();

    return batch.parse(ctx).m_node;
}
}; // namespace InstructionParser
