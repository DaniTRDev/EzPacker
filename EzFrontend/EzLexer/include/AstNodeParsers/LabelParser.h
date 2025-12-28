#ifndef EZPACKER_LABELPARSER_H
#define EZPACKER_LABELPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/Label.h"
#include "AstNodeParsers/InstructionParser.h"

class LabelParser : public IAstNodeParser<Label>
{
  public:
    /**
     * Tries to parse a label out of the token list within context.
     * @return std::shared_ptr<Label>
     */
    std::shared_ptr<Label> parse(const std::shared_ptr<IParsingContext> &ctx) override;
};

#endif // EZPACKER_LABELPARSER_H
