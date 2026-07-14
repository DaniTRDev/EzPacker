#include "Operand/MirOperandBuilder.h"

MirOperandBuilder::MirOperandBuilder(MirBuilderContext *ctx) : m_ctx(ctx) {}

MirFloat *MirOperandBuilder::buildFloat(MirType *type, const FlexFloat &value, SourceReference *ref)
{
    return build<MirFloat>(type, value, ref);
}

MirInteger *MirOperandBuilder::buildInt(MirType *type, const FlexInt &value, SourceReference *ref)
{
    return build<MirInteger>(type, value, ref);
}

MirMemory *MirOperandBuilder::buildMem(MirType *type, MirRegister *base, MirInteger *displ, SourceReference *ref)
{
    return build<MirMemory>(type, base, displ, ref);
}

MirMemory *MirOperandBuilder::buildMem(MirType *type, MirRegister *base, const FlexInt &displ, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    return build<MirMemory>(type, base, build<MirInteger>(t->i64(), displ, nullptr), ref);
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

    return build<MirReference>(ptr, MirReferenceType::Block, block->getId(), 0, ref);
}

MirReference *MirOperandBuilder::buildRef(MirFunction *func, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(func->getReturnType());

    return build<MirReference>(ptr, MirReferenceType::Function, func->getId(), 0, ref);
}

MirReference *MirOperandBuilder::buildRef(MirGlobalDataEntry *entry, size_t offset, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(entry->m_dataType);

    return build<MirReference>(ptr, MirReferenceType::DataEntry, entry->m_entryId, offset, ref);
}

MirReference *MirOperandBuilder::buildRef(MirRegister *structPtr, struct MirStructField *field, SourceReference *ref)
{
    // TODO
    auto &t = m_ctx->getTypeTable();
    MirType *fieldPtrType = t->getPtr(field->m_dataType);

    return build<MirReference>(fieldPtrType, MirReferenceType::StructField, structPtr->getRegId(), field->getId(), ref);
}

MirRuntimeSymbol *MirOperandBuilder::buildRtSymbol(std::pmr::string symbolName, SourceReference *ref)
{
    const auto &t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(t->getVoidType());

    return build<MirRuntimeSymbol>(ptr, std::move(symbolName), ref);
}
