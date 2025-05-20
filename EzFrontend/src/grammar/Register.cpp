#include "grammar/Register.h"

namespace grammar
{
std::shared_ptr<ParseRule> _register()
{
    return combinators::sequence({tokenType(IRTokenType::Percentage), identifier()})->map<RegisterNode>();
}
} // namespace grammar

