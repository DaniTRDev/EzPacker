#include "parser/grammar/Instruction.h"

namespace grammar
{
std::shared_ptr<ParseRule> operand()
{
    return combinators::sequence({type(), combinators::anyOf({virtualVariable(), number(), memory()})});
}

std::shared_ptr<ParseRule> instruction()
{
    return combinators::sequence({tokenType(IRTokenType::Dot), identifier(), operand(),
                                  combinators::matchIf(tokenType(IRTokenType::Comma), operand())})
        ->map<InstructionNode>();
}

} // namespace grammar
