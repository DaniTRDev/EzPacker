/**
 * @file ModuleLowerer.h
 * @brief Lowerers for the Module (function) declaration: header + body.
 *
 * Two lowerer classes are defined here:
 *   - ModuleHeaderLowerer — lowers each parameter declaration in the header
 *     into a virtual register and links the parameter symbol to it.
 *   - ModuleLowerer       — creates the function's entry block, links the
 *     module symbol to it, lowers the header, then lowers the body scope.
 */
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
