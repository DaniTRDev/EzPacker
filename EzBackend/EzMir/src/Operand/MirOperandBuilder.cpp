#include "Operand/MirOperandBuilder.h"

MirOperandBuilder::MirOperandBuilder(MirBuilderContext *ctx) : m_ctx(ctx) {}

MirFloat *MirOperandBuilder::buildFloat(float value, SourceReference *ref)
{
    return build<MirFloat>(m_ctx->getTypeTable()->f32(), std::to_string(value).c_str(), ref);
}

MirFloat *MirOperandBuilder::buildFloat(double value, SourceReference *ref)
{
    return build<MirFloat>(m_ctx->getTypeTable()->f64(), std::to_string(value).c_str(), ref);
}

MirFloat *MirOperandBuilder::buildFloat(MirType *type, std::pmr::string value, SourceReference *ref)
{
    if (type->getKind() != MirTypeKind::FloatingPoint)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                << "Given float type is not a float type" << ref;
        return nullptr;
    }

    return build<MirFloat>(type, std::move(value).c_str(), ref);
}

MirInteger *MirOperandBuilder::buildInt(MirType *type, int64_t value, SourceReference *ref)
{
    return build<MirInteger>(type, value, ref);
}

MirRegister *MirOperandBuilder::buildVReg(MirType *type, std::pmr::string name, SourceReference *ref)
{
    return build<MirRegister>(type, true, m_ctx->createId(), ref, name);
}

MirRegister *MirOperandBuilder::buildPhysReg(MirType *type, std::pmr::string name, SourceReference *ref)
{
    return build<MirRegister>(type, false, m_ctx->createId(), ref, name);
}

MirReference *MirOperandBuilder::buildRef(MirBlock *block, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(t->getVoidType());

    return build<MirReference>(ptr, MirReferenceType::Block, block->getId(), ref);
}

MirReference *MirOperandBuilder::buildRef(MirFunction *func, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(func->getReturnType());

    return build<MirReference>(ptr, MirReferenceType::Function, func->getId(), ref);
}

MirReference *MirOperandBuilder::buildRef(MirGlobalDataEntry *entry, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(entry->m_dataType);

    return build<MirReference>(ptr, MirReferenceType::DataEntry, entry->m_entryId, ref);
}
