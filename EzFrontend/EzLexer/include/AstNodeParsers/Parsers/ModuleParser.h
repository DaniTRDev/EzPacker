/**
 * @file ModuleParser.h
 * @brief Parser for function/module declarations: `returnType Name(params) { body }`.
 *
 * Two parser classes live inside the ModuleParser namespace:
 *   - ModuleHeaderParser — parses the return type, name, and parameter list.
 *   - ModuleParser       — parses the full declaration (header + body scope).
 * A Module is the top-level compilation unit in the language.
 */
#ifndef EZPACKER_MODULEPARSER_H
#define EZPACKER_MODULEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/Module.h"
#include "VariableParser.h"
#include "CodeScopeParser.h"

namespace ModuleParser
{
class ModuleHeaderParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse the header of a module out of the token list within context.
     * @param ctx
     * @return ModuleHeader * (as AstNode *)
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class ModuleParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse an module out of the token list within context.
     * @param ctx
     * @return Module *
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};
}; // namespace ModuleParser
#endif // EZPACKER_MODULEPARSER_H
