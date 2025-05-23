#include "parser/grammar/Variable.h"

namespace grammar
{
std::shared_ptr<ParseRule> initializer()
{
    return combinators::manyOf(
        0, UINT64_MAX,
        combinators::sequence({tokenType(IRTokenType::Comma), combinators::anyOf({number(), string()})}));
}

std::shared_ptr<ParseRule> variable()
{
    return combinators::sequence({keyword(), identifier(), tokenType(IRTokenType::Colon), type(),
                                  combinators::anyOf({number(), string()}), combinators::optional(initializer())})
        ->map<VariableNode>();
}
} // namespace Grammar
