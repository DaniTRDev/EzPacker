#include "Parser/MemoryParsers.h"

namespace NodeParsers
{
ParsingRule &BaseMemory()
{
    static ParsingRule p =
            Rules::Sequence(LeftParen(), VirtualVariable(), RightParen())->encapsulate(&AstNodes::BaseMemory());
    return p;
}

ParsingRule &BaseDisplMemory()
{
    static ParsingRule p = Rules::Sequence(LeftParen(), VirtualVariable(), Comma(), IntNumber(), RightParen())
                                   ->encapsulate(&AstNodes::BaseDisplMemory());
    return p;
}

ParsingRule &BaseIndexScaleDisplMemory()
{
    static ParsingRule p = Rules::Sequence(LeftParen(),
                                           VirtualVariable(),
                                           Comma(),
                                           VirtualVariable(),
                                           Comma(),
                                           IntNumber(),
                                           Comma(),
                                           IntNumber(),
                                           RightParen())
                                   ->encapsulate(&AstNodes::BaseIndexScaleDisplMemory());
    return p;
}

ParsingRule &DirectMemory()
{
    static ParsingRule p =
            Rules::Sequence(LeftParen(), IntNumber(), RightParen())->encapsulate(&AstNodes::DirectMemory());
    return p;
}

ParsingRule &IndexScaleMemory()
{
    static ParsingRule p = Rules::Sequence(LeftParen(), Comma(), VirtualVariable(), Comma(), IntNumber(), RightParen())
                                   ->encapsulate(&AstNodes::IndexScaleMemory());
    return p;
}

ParsingRule &MemoryReference()
{
    static ParsingRule p = Rules::AnyOf(BaseMemory(),
                                        BaseDisplMemory(),
                                        BaseIndexScaleDisplMemory(),
                                        DirectMemory(),
                                        IndexScaleMemory());
    return p;
}
} // namespace NodeParsers
