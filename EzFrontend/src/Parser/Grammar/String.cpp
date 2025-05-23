#include "parser/grammar/String.h"

namespace grammar
{
std::shared_ptr<ParseRule> string()
{
    return combinators::sequence({tokenType(IRTokenType::String)})->map<ValueNode>(ValueNodeType::String);
}
} // namespace Grammar
