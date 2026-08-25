#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterReference.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

MirOperandBuilder::MirOperandBuilder(MirBuilderContext *ctx) :
    m_ctx(ctx), m_resource(ctx->getGlobalAllocator()), m_allocator(m_resource)
{
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
    MirType *baseType = base->getMirType();
    if (baseType->getKind() != MirTypeKind::Pointer)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                << ref << "Can't create a memory operand if the base register doesn't have pointer type "
                << type->getName();
        return nullptr;
    }

    return build<MirMemory>(type, base, displ, ref);
}

MirMemory *MirOperandBuilder::buildMem(MirType *type, MirRegister *base, const FlexInt &displ, SourceReference *ref)
{
    MirType *baseType = base->getMirType();
    if (baseType->getKind() != MirTypeKind::Pointer)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                << ref << "Can't create a memory operand if the base register doesn't have pointer type "
                << type->getName();
        return nullptr;
    }

    MirTypeTable *t = m_ctx->getTypeTable();
    return build<MirMemory>(type, base, build<MirInteger>(t->i64(), displ, nullptr), ref);
}

MirRegister *
MirOperandBuilder::buildVReg(MirType *type, std::pmr::string name, SourceReference *ref, MirRegisterClass *_class)
{
    MirRegister *reg = build<MirRegister>(type, true, m_ctx->createId(), ref, _class, name);
    m_ctx->appendRegister(reg);

    return reg;
}

MirRegister *MirOperandBuilder::buildPhysReg(
        MirType *type, MirPhysicalRegId physId, std::pmr::string name, MirRegisterClass *_class, SourceReference *ref)
{
    return build<MirRegister>(type, false, physId, ref, _class, name);
}

MirReference *MirOperandBuilder::buildRef(MirBlock *block, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(t->_void());

    return build<MirReference>(ptr, MirReferenceType::Block, block->getId(), 0, ref);
}

MirReference *MirOperandBuilder::buildRef(MirFunction *func, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(func->getType());

    return build<MirReference>(ptr, MirReferenceType::Function, func->getId(), 0, ref);
}

MirReference *MirOperandBuilder::buildRef(MirGlobalVar *var, size_t offset, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(var->getType());

    return build<MirReference>(ptr, MirReferenceType::GlobalVar, var->getId(), offset, ref);
}

MirReference *MirOperandBuilder::buildRef(StackFrameObject *obj, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *fieldPtrType = t->getPtr(obj->m_type);

    return build<MirReference>(fieldPtrType, MirReferenceType::StackFrameObject, obj->m_id, 0, ref);
}

MirRuntimeSymbol *MirOperandBuilder::buildRtSymbol(std::pmr::string symbolName, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(t->_void());

    return build<MirRuntimeSymbol>(ptr, std::move(symbolName), ref);
}
