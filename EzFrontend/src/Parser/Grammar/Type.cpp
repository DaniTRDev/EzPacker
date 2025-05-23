#include "parser/grammar/Type.h"

namespace grammar
{
std::shared_ptr<ParseRule> type()
{
    return combinators::sequence({tokenType(IRTokenType::Dot), identifier()})->map<TypeNode>();
}
} // namespace Grammar
