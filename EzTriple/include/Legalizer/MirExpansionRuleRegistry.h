#ifndef EZTRIPLE_MIR_EXPANSION_RULE_REGISTRY_H
#define EZTRIPLE_MIR_EXPANSION_RULE_REGISTRY_H

#include "EzTripleCommon.h"

class MirBuilderContext;
class MirInstruction;

/**
 * Abstract interface for target-specific legalization expansion rewrite rules.
 * Enables targets to customize how illegal or multi-word instructions are expanded into legal primitives.
 */
class MirExpansionRuleRegistry
{
  public:
    virtual ~MirExpansionRuleRegistry() = default;

    /**
     * Attempts to expand an unlowered or illegal instruction using synthesized target rewrite rules.
     * Active MIR builder context.
     * Instruction to expand.
     * True if the instruction was successfully matched and expanded.
     */
    virtual bool tryExpand(MirBuilderContext *ctx, MirInstruction *inst) = 0;
};

#endif // EZTRIPLE_MIR_EXPANSION_RULE_REGISTRY_H
