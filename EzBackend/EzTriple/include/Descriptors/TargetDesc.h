#ifndef EZPACKER_TARGETDESC_H
#define EZPACKER_TARGETDESC_H

#include "EzTripleCommon.h"

/**
 * Interface used to store target-dependent information that is not managed by the OS.
 */
class TargetDesc
{
  public:
    virtual ~TargetDesc() = default;

    /**
     * Returns the name of the target.
     * @return
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the expansion recipes for this target.
     * @return
     */
    virtual const class ExpansionRecipe *getExpansionRecipes() = 0;

    /**
     * Returns the expansion recipe for the given instruction in this target.
     * @return
     */
    virtual const class ExpansionRecipe *const getExpansionRecipeForInstr(MirInstructionOpCode opcode) = 0;

    /**
     * Returns the nearest compatible type for the given type. If the type is already legal, it is returned as-is. If no
     * type can be used, nullptr will be returned.
     *
     * Examples 1: using an i1 (1-bit integer) is not possible in x64 arithmetic instructions, but might be allowed for
     * dev convenience, it must be promoted to the first legal type, which is i8 (8-bit).
     *
     * Example 2: using an i128 is not possible in x64 (without SSE/AVX), the operand needs to be expanded into 2 i64.
     * @param type
     * @return
     */
    virtual MirType *getNearestLegalType(MirType *type) = 0;

    /**
     * Returns the expansion recipes array size.
     * @return
     */
    virtual size_t getExpansionRecipesSize() = 0;
};

#endif // EZPACKER_TARGETDESC_H
