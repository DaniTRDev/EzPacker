#ifndef EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H
#define EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H

#include "Descriptors/TargetDesc.h"
#include "EzTripleTestExpansionRecipe.h"

/**
 * This class acts a simple target descriptor that is already defined. Its purpose is just to acts as an already-defined
 * descriptor for tests.
 *
 * Defined registers:
 *  GPRs: { 1, 2, 3 }
 *  FPRs (Volatile): { 4, 5 }
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
    const ExpansionRecipe *getExpansionRecipes() override { return GET_EXPANSION_RECIPES(EzTripleTest); }

    /**
     * Returns the expansion recipe for the given instruction in this target.
     * @return
     */
    const ExpansionRecipe *const getExpansionRecipeForInstr(MirInstructionOpCode opcode) override
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

    /**
     * Returns the available registers for the given class.
     */
    std::pmr::vector<RegisterRef> getAvailableRegisters(RegisterRefClass refClass) override
    {
        switch (refClass)
        {
            case RegisterRefClass::GPR:
            {
                return std::pmr::vector<RegisterRef>({ RegisterRef::preg(RegisterRefClass::GPR, 1),
                                                       RegisterRef::preg(RegisterRefClass::GPR, 2),
                                                       RegisterRef::preg(RegisterRefClass::GPR, 3) },
                                                     m_ctx->getGlobalAllocator());
            }
            case RegisterRefClass::FPR:
            {
                return std::pmr::vector<RegisterRef>(
                        { RegisterRef::preg(RegisterRefClass::FPR, 4), RegisterRef::preg(RegisterRefClass::FPR, 5) },
                        m_ctx->getGlobalAllocator());
            }
            default:
            {
                throw std::runtime_error("Unsupported register class");
            }
        }
    }

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H
