#include "AstNodeParsers/Parsers/ImmediateParser.h"

namespace ImmediateParser
{
AstNode *Integer::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    mp_int *integer = ctx->getNodePool()->create<mp_int>();
    IntegerImmediate *node;
    TokenInformation immediateToken, typeToken;

    bool isNegative = false;

    ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Plus);
    if (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Minus))
    {
        isNegative = true;
    }

    // Immediate type.
    ctx->consumeIf(ParsingCondition::TokenType, &typeToken, _TokenType::Identifier);
    if (!ctx->consumeIf(ParsingCondition::TokenType, &immediateToken, _TokenType::NumberInt))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected integer value for integer immediate",
                       "ImmediateParser::Integer",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    const std::string &tokenStr = immediateToken.m_str;
    std::string str = (isNegative ? "-" : "") + tokenStr;

    int radix = 10; // Default
    if (tokenStr.starts_with("0x"))
    {
        // hex
        radix = 16;
        str.erase(0, 2); // Remove '0x' from input.
    }

    if (mp_init(integer) != MP_OKAY)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Error with big integer library",
                       "ImmediateParser::Integer",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (mp_read_radix(integer, str.data(), radix) != MP_OKAY)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Integer is malformed",
                       "ImmediateParser::Integer",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    node = ctx->getNodePool()->create<IntegerImmediate>(integer);
    node->setDataType(ctx->getStringPool()->createConstantString(typeToken.m_str));

    return node;
}

AstNode *Float::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    double value = 0;
    TokenInformation token;
    TokenInformation immediateToken, typeToken;

    // Immediate type.
    ctx->consumeIf(ParsingCondition::TokenType, &typeToken, _TokenType::Identifier);
    if (!ctx->consumeIf(ParsingCondition::TokenType, &token, _TokenType::NumberFloat))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected float value for float immediate",
                       "ImmediateParser::Float",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::string &str = token.m_str;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc())
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Could not convert float to bytes",
                       "ImmediateParser::Float",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    FloatImmediate *node = ctx->getNodePool()->create<FloatImmediate>(value);
    node->setDataType(ctx->getStringPool()->createConstantString(typeToken.m_str));

    return node;
}

AstNode *String::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation token;

    if (!ctx->consumeIf(ParsingCondition::TokenType, &token, _TokenType::String))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected string value for string immediate",
                       "ImmediateParser::String",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::string_view str = ctx->getStringPool()->createConstantString(token.m_str);
    StringImmediate *node = ctx->getNodePool()->create<StringImmediate>(std::move(str));

    return node;
}

ParserBatch CreateImmediateParserBatch()
{
    ParserBatch batch;
    batch.addParsersFromTypeList<Integer, Float, String>();

    return batch;
}

AstNode *ImmediateParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    ParserBatch batch;
    batch.addParsersFromTypeList<Integer, Float, String>();

    return batch.parse(ctx).m_node;
}
}; // namespace ImmediateParser