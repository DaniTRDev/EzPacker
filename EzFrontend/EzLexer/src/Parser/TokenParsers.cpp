#include "Parser/TokenParsers.h"

namespace NodeParsers
{
ParsingRule &Colon()
{
    static auto p = Token(_TokenType::Colon);
    return p;
}

ParsingRule &Comma()
{
    static auto p = Token(_TokenType::Comma);
    return p;
}

ParsingRule &Dot()
{
    static auto p = Token(_TokenType::Dot);
    return p;
}

ParsingRule &Percentage()
{
    static auto p = Token(_TokenType::Percentage);
    return p;
}

ParsingRule &LeftBrace()
{
    static auto p = Token(_TokenType::LeftBrace);
    return p;
}

ParsingRule &LeftParen()
{
    static auto p = Token(_TokenType::LeftParen);
    return p;
}

ParsingRule &RightBrace()
{
    static auto p = Token(_TokenType::RightBrace);
    return p;
}

ParsingRule &RightParen()
{
    static auto p = Token(_TokenType::RightParen);
    return p;
}

ParsingRule &SemiColon()
{
    static auto p = Token(_TokenType::SemiColon);
    return p;
}

ParsingRule &Identifier()
{
    static auto p = Token(_TokenType::Identifier, &AstNodes::Identifier());
    return p;
}

ParsingRule &IntNumber()
{
    static auto p = Token(_TokenType::NumberInt, &AstNodes::IntNumber());
    return p;
}

ParsingRule &FloatNumber()
{
    static auto p = Token(_TokenType::NumberFloat, &AstNodes::FloatNumber());
    return p;
}

ParsingRule &String()
{
    static auto p = Token(_TokenType::String, &AstNodes::String());
    return p;
}

ParsingRule Token(_TokenType type, AstNodeBuilder *builder)
{
    return Rule::create(
            [type, builder](IParsingContext &ctx, const std::shared_ptr<AstNode> &out) -> bool
            {
                auto &token = ctx.peek();
                if (token.m_type != type)
                {
                    ctx.getErrorCollector()->enterScope();
                    ctx.getErrorCollector()->error(LogMessage("Expected {} but got {}",
                                                              TokenType2StrMap[type],
                                                              TokenType2StrMap[token.m_type]),
                                                   ctx.peek().m_sourceReference);
                    ctx.getErrorCollector()->exitScope(ErrorHandleType::Propagate);
                    return false;
                }

                ctx.consume();
                if (builder)
                {
                    std::shared_ptr<AstNode> node = builder->build();
                    node->setContent(token.m_str);
                    node->setSourceRef(token.m_sourceReference);

                    out->addChild(node);
                }

                return true;
            });
}
} // namespace NodeParsers