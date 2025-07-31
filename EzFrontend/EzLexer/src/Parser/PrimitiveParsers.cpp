#include "Parser/PrimitiveParsers.h"

namespace NodeParsers
{
ParsingRule Keyword(const std::string &kw)
{
    return Rules::Sequence(Dot())->then(
            [kw](IParsingContext &parser, const std::shared_ptr<AstNode> &out) -> bool
            {
                std::shared_ptr<AstNode> node = AstNodes::Null().build(); // Dummy node.
                parser.getErrorCollector()->enterScope();
                if (!Identifier()->match(parser, node))
                {
                    parser.getErrorCollector()->error(LogMessage("Expected keyword identifier"));
                    parser.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                    return false;
                }
                else if (node->getChild(0)->getContent() != kw)
                {
                    parser.getErrorCollector()->error(LogMessage("Expected '{}' but got '{}'", kw, node->getContent()));
                    parser.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                    return false;
                }

                parser.getErrorCollector()->exitScope(ErrorHandleType::Discard);
                return true;
            });
}

ParsingRule &Number()
{
    static auto p = Rules::AnyOf(IntNumber(), FloatNumber());
    return p;
}

ParsingRule &Type()
{
    static auto p = Rules::Sequence(Dot(), Token(_TokenType::Identifier, &AstNodes::Type()));
    return p;
}

ParsingRule &Variable()
{
    static auto p =
            Rules::Sequence(Keyword("variable"),
                            Identifier(),
                            Colon(),
                            Type(),
                            Rules::AnyOf(Number(), String()),
                            Rules::StrictManyOf(Comma(), Rules::Sequence(Comma(), Rules::AnyOf(Number(), String()))),
                            SemiColon())
                    ->encapsulate(&AstNodes::Variable());
    return p;
}

ParsingRule &VirtualVariable()
{
    static auto p =
            Rules::Sequence(Percentage())
                    ->then(
                            [](IParsingContext &parser, const std::shared_ptr<AstNode> &out) -> bool
                            {
                                std::shared_ptr<AstNode> node = AstNodes::VirtualVariable().build();
                                if (!Identifier()->match(parser, node))
                                {
                                    parser.getErrorCollector()->enterScope();
                                    parser.getErrorCollector()->error(LogMessage("Expected name for virtual variable"));
                                    parser.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                                    return false;
                                }
                                out->addChild(std::move(node));
                                return true;
                            });
    return p;
}

} // namespace NodeParsers