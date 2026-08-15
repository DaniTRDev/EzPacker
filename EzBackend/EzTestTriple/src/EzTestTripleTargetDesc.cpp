#include "EzTestTripleTargetDesc.h"

EzTestTripleTargetDesc::EzTestTripleTargetDesc(MirBuilderContext *ctx, std::pmr::memory_resource *alloc) :
    m_ctx(ctx), m_alloc(alloc), m_registerBanks(alloc), m_callingConvs(alloc)
{
}

MirType *EzTestTripleTargetDesc::getMemOperandDisplacementType()
{
    // 64-bit signed integer displacement for memory indexing
    return m_ctx->getTypeTable()->i64();
}

MirType *EzTestTripleTargetDesc::getNearestLegalType(MirType *type)
{
    if (!type)
        return nullptr;

    const auto &t = m_ctx->getTypeTable();
    switch (type->getKind())
    {
        case MirTypeKind::Integer:
        {
            size_t bits = type->getTotalSizeInBits();
            if (bits <= 8)
                return t->i8();
            if (bits <= 16)
                return t->i16();
            if (bits <= 32)
                return t->i32();
            if (bits <= 64)
                return t->i64();
            return t->i64(); // Wider integers get legalized via multi-precision expansion
        }
        case MirTypeKind::FloatingPoint:
        {
            size_t bits = type->getTotalSizeInBits();
            if (bits <= 32)
                return t->f32();
            return t->f64();
        }
        default:
            return type;
    }
}

void EzTestTripleTargetDesc::initialize()
{
    // 1. Initialize Register Banks
    m_registerBanks = EzTestTriple::CreateRegisterBanks(m_alloc);

    MirRegisterBank *gprBank = nullptr;
    MirRegisterBank *fprBank = nullptr;
    for (auto *bank : m_registerBanks)
    {
        if (std::string_view(bank->getName()) == "GPR")
            gprBank = bank;
        if (std::string_view(bank->getName()) == "FPR")
            fprBank = bank;
    }

    std::pmr::polymorphic_allocator<> alloc(m_alloc);

    // 2. Initialize Calling Conventions
    auto *defaultCC = alloc.new_object<EzTestTripleCallingConv>(gprBank, fprBank, m_alloc);
    m_callingConvs.push_back(defaultCC);

    // 3. Initialize Frame Lowerer
    m_frameLowerer = alloc.new_object<EzTestTripleFrameLowerer>();

    // 4. Initialize Expansion Rule Registry
    m_expansionRegistry = alloc.new_object<MirExpansionRuleRegistry>(m_ctx);
    EzTestTriple::CreateExpansionRegistry(m_expansionRegistry);

    // 5. Initialize Legalizer
    m_legalizer = alloc.new_object<MirLegalizer>(m_ctx);
    EzTestTriple::CreateLegalizer(m_ctx, m_legalizer);

    // 6. Initialize Instruction Selector
    m_instructionSelector = alloc.new_object<MirInstructionSelector>(m_ctx);
    EzTestTriple::CreateInstructionSelector(m_ctx, m_instructionSelector);
}