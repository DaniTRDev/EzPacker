#include "AstNodeParsers/ImmediateOperandParser.h"

std::shared_ptr<ImmediateOperand> ImmediateOperandParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    return std::dynamic_pointer_cast<ImmediateOperand>(
            AstNodeParsingUtils::tryParsers<Integer, Float, String>(ctx, LogMessage("Invalid immediate")));
}

std::shared_ptr<IntegerImmediate> ImmediateOperandParser::Integer::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    mp_int integer;
    std::shared_ptr<IntegerImmediate> node;
    std::string str;

    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::NumberInt,
                                                  str,
                                                  ctx,
                                                  LogMessage("Expected integer for integer immediate"));

    int radix = 10; // Default
    if (str.starts_with("0x"))
    {
        // hex
        radix = 16;
        str.erase(0, 2); // Remove '0x' from input.
    }

    if (mp_init(&integer) != MP_OKAY)
    {
        return nullptr;
    }

    if (mp_read_radix(&integer, str.data(), radix) != MP_OKAY)
    {
        return nullptr;
    }

    node = std::make_shared<IntegerImmediate>(std::move(integer));
    return std::move(node);
}

std::shared_ptr<FloatImmediate> ImmediateOperandParser::Float::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    double value = 0;
    std::shared_ptr<FloatImmediate> node;
    std::string str;

    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::NumberFloat,
                                                  str,
                                                  ctx,
                                                  LogMessage("Expected float for float immediate"));

    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc())
    {
        return nullptr;
    }

    node = std::make_shared<FloatImmediate>(value);
    return std::move(node);
}

std::shared_ptr<StringImmediate> ImmediateOperandParser::String::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<StringImmediate> node;
    std::string str;

    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::String,
                                                  str,
                                                  ctx,
                                                  LogMessage("Expected string for string immediate"));

    node = std::make_shared<StringImmediate>(std::move(str));
    return std::move(node);
}
