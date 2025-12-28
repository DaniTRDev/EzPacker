#ifndef EZPACKER_MODULEPARSER_H
#define EZPACKER_MODULEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/Module.h"
#include "AstNodeParsers/VariableParser.h"
#include "AstNodeParsers/InstructionParser.h"
#include "AstNodeParsers/LabelParser.h"

class ModuleParser : public IAstNodeParser<Module>
{
  public:
    /**
     * Tries to parse an module out of the token list within context.
     * @param ctx
     * @return std::shared_ptr<Module>
     */
    std::shared_ptr<Module> parse(const std::shared_ptr<IParsingContext> &ctx) override;

    class ModuleBodyParser : public IAstNodeParser<ModuleBody>
    {
      public:
        /**
         * Tries to parse the body of a module out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<::ModuleHeaderParser>
         */
        std::shared_ptr<ModuleBody> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };

    class ModuleHeaderParser : public IAstNodeParser<ModuleHeader>
    {
      public:
        /**
         * Tries to parse the header of a module out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<::ModuleHeaderParser>
         */
        std::shared_ptr<ModuleHeader> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };
};

#endif // EZPACKER_MODULEPARSER_H
