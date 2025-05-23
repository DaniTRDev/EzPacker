#include "parser/grammar/Identifier.h"

namespace grammar
{
std::shared_ptr<ParseRule> identifier()
{
    return combinators::sequence({tokenType(IRTokenType::Identifier)})->map<IdentifierNode>();
}
} // namespace Grammar