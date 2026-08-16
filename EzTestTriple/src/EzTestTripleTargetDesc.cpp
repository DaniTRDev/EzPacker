#include "EzTestTripleTargetDesc.h"

EzTestTripleTargetDesc::EzTestTripleTargetDesc(MirBuilderContext *ctx, std::pmr::memory_resource *alloc) :
    m_ctx(ctx), m_alloc(alloc), m_registerBanks(alloc), m_callingConvs(alloc), m_binDescriptors(alloc)
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

RegisterRef EzTestTripleTargetDesc::getInstructionPtrReg() const { return RegisterRef::preg(m_spr64->getReg("rip")); }

void EzTestTripleTargetDesc::initialize()
{
    using namespace EzTestTriple;
    std::pmr::polymorphic_allocator<> alloc(m_alloc);

    // 1. Initialize Register Banks
    m_registerBanks = EzTestTriple::CreateRegisterBanks(m_alloc);
    m_spr64 = Banks::SPR->getClass("SPR64");

    // 2. Initialize Calling Conventions
    auto *defaultCC = alloc.new_object<EzTestTripleCallingConv>(Banks::GPR, Banks::FPR, m_alloc);
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

    // 7. Initialize Register Allocator
    m_registerAllocator = alloc.new_object<EzTestTripleRegisterAllocator>();

    // 8. Initialize Binary Descriptors.
    auto *defaultBinDesc = alloc.new_object<EzTestTripleBinaryDesc>(EzTestTripleBinaryDesc::Options{}, m_alloc);
    m_binDescriptors.push_back(defaultBinDesc);
}