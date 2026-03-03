#ifndef EZPACKER_LABELPARSER_H
#define EZPACKER_LABELPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/Label.h"
#include "InstructionParser.h"
#include "CodeScopeParser.h"

class LabelParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a label out of the token list within context.
     * @return Label*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_LABELPARSER_H
