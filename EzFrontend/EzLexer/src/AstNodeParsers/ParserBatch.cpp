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
            ctx->beginMultiSourceRef();

            if (auto node = parser->parse(ctx); node)
            {
                /*
                 * This ternary operator is needed, there's a few cases in which the current multireference is already
                 * saved in the node: A batch of type <Label, InstructionParser>, the instruction parser will already
                 * return a node with source references, meaning the multireference created in the for loop for the
                 * parsers has empty elements.
                 */
                std::vector<std::shared_ptr<SourceReference>> sourceRefs = ctx->getCurrentMultiReference().empty()
                        ? node->getSourceRefs()
                        : ctx->getCurrentMultiReference();
                node->setSourceRef(sourceRefs);

                ctx->getErrorCollector()->endScope(ErrorAction::Discard); // Discard this scope.
                ctx->endMultiSourceRef();

                return ParserBatchResult{ .m_node = std::move(node), .m_parser = parser };
            }

            if (ctx->getErrorCollector()->doesCurrentScopeHasFatalErrors())
            {
                // Stop trying parsers, last node returned because it was malformed.
                ctx->getErrorCollector()->endScope(ErrorAction::Propagate);
                ctx->endMultiSourceRef();
                break;
            }

            ctx->getErrorCollector()->endScope(ErrorAction::Discard); // Move to the scope begun in the first line.
            ctx->endMultiSourceRef();
            ctx->setPosition(currentPos); // Reset parser context position prior to the initial point.
        }
    }

    return ParserBatchResult{ .m_node = nullptr, .m_parser = nullptr };
}
