#include "grammar/Module.h"

namespace grammar
{
std::shared_ptr<ParseRule> module()
{
    return combinators::sequence({keyword(), identifier(), tokenType(IRTokenType::Colon),
                                  combinators::manyOf(0, UINT64_MAX, instruction()), keyword()})
        ->map<ModuleNode>();
}
} // namespace grammar