#include "AstNodeParsers/ParserBatch.h"

void ParserBatch::addParser(std::shared_ptr<IAstNodeParser> parser)
{
    if (!parser)
    {
        throw std::runtime_error("ParserBatch supplied with an invalid parser!");
    }

    m_parsers.push_back(std::move(parser));
}

ParserBatchResult ParserBatch::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (ctx->canPeek())
    {
        for (auto &parser : m_parsers)
        {
            size_t currentPos = ctx->getCurrentPosition();
            ctx->getErrorCollector()->beginScope();

            if (auto node = parser->parse(ctx); node)
            {
                /*
                 * There are some nodes that doesn't actually have a source reference. They merely act as
                 * "semantic containers". This is needed to have at least 1 source reference to be able to track the
                 * element.
                 */

                if (!node->getSourceRef().m_valid)
                {
                    node->setSourceRefs(ctx->getLastSourceReference());
                }

                ctx->getErrorCollector()->endScope(ErrorAction::Discard); // Discard this scope.
                return ParserBatchResult{ .m_node = node, .m_parser = parser };
            }

            if (ctx->getErrorCollector()->doesCurrentScopeHasFatalErrors())
            {
                // Stop trying parsers, last node returned because it was malformed.
                ctx->getErrorCollector()->endScope(ErrorAction::Propagate);
                break;
            }

            ctx->getErrorCollector()->endScope(ErrorAction::Discard); // Move to the scope begun in the first line.
            ctx->setPosition(currentPos); // Reset parser context position prior to the initial point.
        }
    }

    return ParserBatchResult{ .m_node = nullptr, .m_parser = nullptr };
}
