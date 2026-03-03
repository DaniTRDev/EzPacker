#ifndef EZPACKER_MODULELOWERER_H
#define EZPACKER_MODULELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"
#include "VariableLowerer.h"

class ModuleHeaderLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given ModuleHeader node with the given lowering context. Will lower the parameters by register
     * them as virtual registers.
     * @param node
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

class ModuleLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given Module node with the given lowering context. Will lower the parameters (into
     * virtual registers), subexpressions and instructions.
     * @param node
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_MODULELOWERER_H
