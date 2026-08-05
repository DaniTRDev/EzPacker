#include "EzTripleTestTargetDescriptor.h"

EzTripleTestTargetDesc::EzTripleTestTargetDesc(MirBuilderContext *ctx)
{
    m_ctx = ctx;
    m_frameLowerer = std::make_shared<EzTripleTestFrameLowerer>();
}

const char *EzTripleTestTargetDesc::getName() const { return "EzTripleTestTargetDesc"; }

const ExpansionRecipe *EzTripleTestTargetDesc::getExpansionRecipes() { return GET_EXPANSION_RECIPES(EzTripleTest); }

const ExpansionRecipe *const EzTripleTestTargetDesc::getExpansionRecipeForInstr(MirInstructionOpCode opcode)
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

MirFrameLowerer *EzTripleTestTargetDesc::getFrameLowerer() { return m_frameLowerer.get(); }

MirType *EzTripleTestTargetDesc::getMemOperandDisplacementType() { return m_ctx->getTypeTable()->i32(); }

MirType *EzTripleTestTargetDesc::getNearestLegalType(MirType *type)
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

size_t EzTripleTestTargetDesc::getExpansionRecipesSize() { return GET_EXPANSION_RECIPES_SIZE(EzTripleTest); }

size_t EzTripleTestTargetDesc::getStackSlotSize() const { return 4; }

std::pmr::vector<RegisterRef> EzTripleTestTargetDesc::getAvailableRegisters(RegisterRefClass refClass)
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
