#include "parser/grammar/Module.h"

namespace grammar
{
std::shared_ptr<ParseRule> module()
{
    return combinators::sequence(
               {keyword(), identifier(), tokenType(IRTokenType::LeftParen),
                combinators::optional(combinators::sequence(
                    {parameter(),
                     combinators::manyOf(0, UINT64_MAX,
                                         combinators::sequence({tokenType(IRTokenType::Comma), parameter()}))})),
                tokenType(IRTokenType::RightParen), combinators::manyOf(1, UINT64_MAX, instruction()), keyword()})
        ->map<ModuleNode>();
}

std::shared_ptr<ParseRule> parameter()
{
    return combinators::sequence({type(), virtualVariable()})->map<ModuleParameterNode>();
}
} // namespace grammar