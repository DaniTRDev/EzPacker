#include "EzTripleTestTargetDescriptor.h"

EzTripleTestTargetDesc::EzTripleTestTargetDesc(MirBuilderContext *ctx)
{
    m_ctx = ctx;
    m_frameLowerer = std::make_shared<EzTripleTestFrameLowerer>();
    m_expansionRegistry = std::make_shared<MirExpansionRuleRegistry>(ctx);

    EzTripleTestExpansionRegistry::create(m_expansionRegistry.get());
}

const char *EzTripleTestTargetDesc::getName() const { return "EzTripleTestTargetDesc"; }

MirExpansionRuleRegistry *EzTripleTestTargetDesc::getExpansionRegistry() { return m_expansionRegistry.get(); }

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
