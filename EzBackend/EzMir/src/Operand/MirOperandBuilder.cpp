#include "Operand/MirOperandBuilder.h"

MirOperandBuilder::MirOperandBuilder(MirBuilderContext *ctx) : m_ctx(ctx) {}

MirConstantArray *
MirOperandBuilder::buildConstantArray(MirType *elemType, const std::vector<MirOperand *> &elems, SourceReference *ref)
{
    MirType *arrType = m_ctx->getTypeTable()->getArray(elemType, elems.size());
    for (auto &elem : elems)
    {
        if (elem->getMirType()->getId() != elemType->getId())
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref << "Can't build constant array because 1 elem does not match expected type";
            return nullptr;
        }
    }

    std::pmr::vector<MirOperand *> ops(m_ctx->getGlobalAllocator());
    ops.insert(ops.begin(), elems.begin(), elems.end());

    return build<MirConstantArray>(arrType, std::move(ops), ref);
}

MirFloat *MirOperandBuilder::buildFloat(MirType *type, const FlexFloat &value, SourceReference *ref)
{
    FlexFloat val = value;
    MirType *destType = type;
    size_t mirSize = type->getTotalSizeInBits(), valueSize = value.getBitSize();

    if (type->getKind() != MirTypeKind::FloatingPoint)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                << ref << "Given float's MirType is not a floating point value: " << type->getName();
        return nullptr;
    }

    if (mirSize > valueSize)
    {
        // Emit a warning an extend.
        m_ctx->getDiagCollector()->builder(Diag_Warning, "MirOperandBuilder")
                << ref << "Extending float value to type: " << type->getName();
        val.extend(mirSize);
    }
    else if (mirSize < valueSize)
    {
        destType = m_ctx->getTypeTable()->getFloatingTypeBySize(valueSize);
        if (destType)
        {
            m_ctx->getDiagCollector()->builder(Diag_Warning, "MirOperandBuilder")
                    << ref << "Promoting float type to type: " << destType->getName();
        }
        else
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref
                    << "Given value's bit-width is BIGGER than internal type and there's no available type to be "
                       "promoted "
                       "to: "
                    << type->getName();
            return nullptr;
        }
    }

    return build<MirFloat>(destType, std::move(val), ref);
}

MirInteger *MirOperandBuilder::buildInt(MirType *type, const FlexInt &value, SourceReference *ref)
{
    FlexInt val = value;
    MirType *destType = type;
    size_t mirSize = type->getTotalSizeInBits(), valueSize = value.getBitSize();

    if (type->getKind() != MirTypeKind::Integer)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                << ref << "Given int's MirType is not an integer: " << type->getName();
        return nullptr;
    }

    if (mirSize > valueSize)
    {
        // Emit a warning an extend.
        m_ctx->getDiagCollector()->builder(Diag_Warning, "MirOperandBuilder")
                << ref << "Z-Extending integer value to type: " << type->getName();
        val.extend(mirSize, false);
    }
    else if (mirSize < valueSize)
    {
        destType = m_ctx->getTypeTable()->getIntegerTypeBySize(valueSize);
        if (destType)
        {
            m_ctx->getDiagCollector()->builder(Diag_Warning, "MirOperandBuilder")
                    << ref << "Promoting integer type to type: " << destType->getName();
        }
        else
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref
                    << "Given value's bit-width is BIGGER than internal type and there's no available type to be "
                       "promoted "
                       "to: "
                    << type->getName();
            return nullptr;
        }
    }

    return build<MirInteger>(destType, std::move(val), ref);
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

MirReference *MirOperandBuilder::buildArrayElemRef(MirGlobalVar *var, size_t elementIndex, SourceReference *ref)
{
    MirType *elementType = var->getType()->getArrayElementType();
    if (!elementType)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                << ref << "Can't create a reference to a global array element because given variable is not an array";
        return nullptr;
    }

    return build<MirReference>(elementType, MirReferenceType::GlobalArrayElem, var->getId(), elementIndex, ref);
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

MirReference *MirOperandBuilder::buildRef(MirGlobalVar *var, size_t offset, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(var->getType());

    return build<MirReference>(ptr, MirReferenceType::GlobalVar, var->getId(), offset, ref);
}

MirReference *MirOperandBuilder::buildRef(MirRegister *classPtr, MirClassField *field, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    MirType *fieldPtrType = t->getPtr(field->m_type);

    return build<MirReference>(fieldPtrType, MirReferenceType::ClassField, classPtr->getRegId(), field->m_id, ref);
}

MirReference *MirOperandBuilder::buildRef(MirRegister *classPtr, MirClassMethod *method, SourceReference *ref)
{
    auto &t = m_ctx->getTypeTable();
    MirType *fieldPtrType = t->getPtr(method->m_func->getReturnType());

    return build<MirReference>(fieldPtrType, MirReferenceType::ClassMethod, classPtr->getRegId(), method->m_id, ref);
}

MirRuntimeSymbol *MirOperandBuilder::buildRtSymbol(std::pmr::string symbolName, SourceReference *ref)
{
    const auto &t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(t->getVoidType());

    return build<MirRuntimeSymbol>(ptr, std::move(symbolName), ref);
}
