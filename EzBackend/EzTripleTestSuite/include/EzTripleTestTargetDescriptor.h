#ifndef EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H
#define EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H

#include "Descriptors/TargetDesc.h"
#include "EzTripleTestExpansionRecipe.h"

/**
 * This class acts a simple target descriptor that is already defined. Its purpose is just to acts as an already-defined
 * descriptor for tests.
 */
class EzTripleTestTargetDesc : public TargetDesc
{
  public:
    EzTripleTestTargetDesc(MirBuilderContext *ctx) { m_ctx = ctx; }

    /**
     * Returns "EzTripleTargetDesc"
     * @return
     */
    const char *getName() const override { return "EzTripleTestTargetDesc"; }

    /**
     * Returns the expansion recipes for this target.
     * @return
     */
    const class ExpansionRecipe *getExpansionRecipes() override { return GET_EXPANSION_RECIPES(EzTripleTest); }

    /**
     * Returns the expansion recipe for the given instruction in this target.
     * @return
     */
    const class ExpansionRecipe *const getExpansionRecipeForInstr(MirInstructionOpCode opcode) override
    {
        const auto recipes = getExpansionRecipes();
        for (size_t i = 0; i < getExpansionRecipesSize(); i++)
        {
            const auto recipe = &recipes[i];
            if (recipe->m_target == opcode)
                return recipe;
        }

        return nullptr;
    }

    /**
     * This function mimics the x64 target description.
     * @param type
     * @return
     */
    MirType *getNearestLegalType(MirType *type) override
    {
        const auto &t = m_ctx->getTypeTable();

        if (type->getKind() == MirTypeKind::Integer)
        {
            if (type->getTotalSizeInBits() <= 8)
                return t->i8();

            if (type->getTotalSizeInBits() <= 16)
                return t->i16();

            if (type->getTotalSizeInBits() <= 32)
                return t->i32();

            /**
             * This ensure promotion for values between 33 and 64 bits and expansion for values bigger than 64 bits.
             */
            return t->i64();
        }
        else if (type->getKind() == MirTypeKind::FloatingPoint)
        {
            if (type->getTotalSizeInBits() <= 32)
                return t->f32();

            /**
             * This ensure promotion for values between 33 and 64 bits and expansion for values bigger than 64 bits.
             */
            return t->f64();
        }

        return nullptr;
    }

    size_t getExpansionRecipesSize() override { return GET_EXPANSION_RECIPES_SIZE(EzTripleTest); }

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H
