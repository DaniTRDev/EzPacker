#include "parser/grammar/VirtualVariable.h"

namespace grammar
{
std::shared_ptr<ParseRule> virtualVariable()
{
    return combinators::sequence({tokenType(IRTokenType::Percentage), identifier()})->map<VirtualVariableNode>();
}
} // namespace grammar
