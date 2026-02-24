#ifndef EZPACKER_CODESCOPEPARSER_H
#define EZPACKER_CODESCOPEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/CodeScope.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodeParsers/ParserBatch.h"
#include "AstNodeParsers/Parsers/LabelParser.h"
#include "AstNodeParsers/Parsers/InstructionParser.h"

class CodeScopeParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse the code of a scope.
     * @param ctx
     * @return std::shared_ptr<::ModuleHeaderParser>
     */
    std::shared_ptr<AstNode> parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_CODESCOPEPARSER_H
