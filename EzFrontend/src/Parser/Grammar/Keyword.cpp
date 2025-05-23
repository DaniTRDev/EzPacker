#include "parser/grammar/Keyword.h"

namespace grammar
{
std::shared_ptr<ParseRule> keyword()
{
    return combinators::sequence({tokenType(IRTokenType::Dot), identifier()});
}
} // namespace Grammar
