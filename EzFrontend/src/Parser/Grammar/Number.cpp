#include "parser/grammar/Number.h"

namespace grammar
{

std::shared_ptr<ParseRule> numberFloat()
{
    return tokenType(IRTokenType::NumberFloat)->map<ValueNode>(ValueNodeType::Float);
}

std::shared_ptr<ParseRule> numberInt()
{
    return tokenType(IRTokenType::NumberInt)->map<ValueNode>(ValueNodeType::Int);
}

std::shared_ptr<ParseRule> number()
{
    return combinators::anyOf({numberFloat(), numberInt()});
}

} // namespace Grammar
