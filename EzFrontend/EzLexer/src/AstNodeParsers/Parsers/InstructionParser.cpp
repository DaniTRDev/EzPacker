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

AstNode *CallInstructionParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation instructionToken, calleeNameToken, returnTypeToken;

    CallInstruction *node = nullptr;
    StringPool *stringPool = ctx->getStringPool();
    TypedPool *nodePool = ctx->getNodePool();
    TypedPoolSlice<AstNode> *arguments = nodePool->createSlice<AstNode>();

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

    std::string_view calleeName = stringPool->createConstantString(calleeNameToken.m_str),
                     returnType = stringPool->createConstantString(returnTypeToken.m_str);

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {
        // We expect call parameters.
        ParserBatch batch = CreateInstructionOperandBatch();
        do
        {
            AstNode *argument = batch.parse(ctx).m_node;
            if (!argument)
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Invalid expression in call argument",
                               "InstructionParser::CallInstructionParser",
                               ctx->getLastSourceReference());

                return nullptr;
            }

            nodePool->appendToSlice(arguments, argument);
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

    node = nodePool->create<CallInstruction>(arguments, std::move(calleeName), std::move(returnType));
    return node;
}

AstNode *NonCallInstructionParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation token;
    Instruction *node = nullptr;
    StringPool *stringPool = ctx->getStringPool();
    TypedPool *nodePool = ctx->getNodePool();
    TypedPoolSlice<AstNode> *operands = nodePool->createSlice<AstNode>();

    if (!ctx->consumeIf(ParsingCondition::TokenType, &token, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected identifier for instruction mnemonic",
                       "InstructionParser::NonCallInstructionParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::string instructionNameToLower = token.m_str;
    std::ranges::transform(instructionNameToLower,
                           instructionNameToLower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::string_view instructionName = stringPool->createConstantString(instructionNameToLower);

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::SemiColon))
    {
        // At least 1 operand is expected before the semicolon (';')
        static ParserBatch batch = CreateInstructionOperandBatch();
        do
        {
            AstNode *operand = batch.parse(ctx).m_node;
            if (!operand)
            {
                ctx->emitError(ErrorSeverity::Soft,
                               "Invalid expression on instruction operand",
                               "InstructionParser::NonCallInstructionParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }
            nodePool->appendToSlice(operands, operand);
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

    node = nodePool->create<Instruction>(operands, std::move(instructionName));
    return node;
}

AstNode *InstructionParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    ParserBatch batch;
    batch.addParsersFromTypeList<CallInstructionParser, NonCallInstructionParser>();

    return batch.parse(ctx).m_node;
}
}; // namespace InstructionParser
