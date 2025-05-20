#include "grammar/Addressing.h"

namespace grammar
{
std::shared_ptr<ParseRule> base()
{
    return combinators::sequence({tokenType(IRTokenType::LeftParen), _register(), tokenType(IRTokenType::RightParen)})
        ->map<MemoryNode>(IRMemoryReferenceType::Base);
}

std::shared_ptr<ParseRule> baseDispl()
{
    return combinators::sequence({tokenType(IRTokenType::LeftParen), _register(), tokenType(IRTokenType::Comma),
                                  number(), tokenType(IRTokenType::RightParen)})
        ->map<MemoryNode>(IRMemoryReferenceType::BaseDisplacement);
}

std::shared_ptr<ParseRule> baseIndexScaleDisplacement()
{
    return combinators::sequence({tokenType(IRTokenType::LeftParen),
                                  _register(), // base
                                  tokenType(IRTokenType::Comma),
                                  _register(), // index
                                  tokenType(IRTokenType::Comma),
                                  number(), // scale factor
                                  tokenType(IRTokenType::Comma),
                                  number(), // displacement
                                  tokenType(IRTokenType::RightParen)})
        ->map<MemoryNode>(IRMemoryReferenceType::BaseIndexScaleDisplacement);
}

std::shared_ptr<ParseRule> direct()
{
    return combinators::sequence({tokenType(IRTokenType::LeftParen), number(), tokenType(IRTokenType::RightParen)})
        ->map<MemoryNode>(IRMemoryReferenceType::Direct);
}

std::shared_ptr<ParseRule> indexScale()
{
    return combinators::sequence({tokenType(IRTokenType::LeftParen), tokenType(IRTokenType::Comma), _register(),
                                  tokenType(IRTokenType::Comma), number(), tokenType(IRTokenType::RightParen)})
        ->map<MemoryNode>(IRMemoryReferenceType::IndexScale);
}

std::shared_ptr<ParseRule> memory()
{
    return combinators::sequence({type(), combinators::anyOf({base(), baseDispl(), baseIndexScaleDisplacement(),
                                                              direct(), indexScale()})});
}
} // namespace grammar::addressing
