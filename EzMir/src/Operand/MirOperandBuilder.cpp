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

/**
 * Initializes the builder with the parent context and retrieves its global allocator resource.
 */
MirOperandBuilder::MirOperandBuilder(MirBuilderContext *ctx) :
    m_ctx(ctx), m_resource(ctx->getGlobalAllocator()), m_allocator(m_resource)
{
}

/**
 * Builds a floating-point constant operand, extending precision or promoting type to match target width.
 */
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
        // Emit a warning and extend.
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

/**
 * Builds an integer constant operand, zero-extending or promoting type to match target width.
 */
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
        // Emit a warning and extend.
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

/**
 * Constructs a base-plus-displacement memory operand using an existing MirInteger displacement operand.
 */
MirMemory *MirOperandBuilder::buildMem(MirType *type, MirRegister *base, MirInteger *displ, SourceReference *ref)
{
    if (base)
    {
        MirType *baseType = base->getMirType();
        if (baseType->getKind() != MirTypeKind::Pointer)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref << "Can't create a memory operand if the base register doesn't have pointer type "
                    << type->getName();
            return nullptr;
        }
    }

    return build<MirMemory>(type, base, displ, nullptr, 1, ref);
}

/**
 * Constructs a base-plus-displacement memory operand converting an immediate FlexInt offset to a 64-bit integer
 * operand.
 */
MirMemory *MirOperandBuilder::buildMem(MirType *type, MirRegister *base, const FlexInt &displ, SourceReference *ref)
{
    if (base)
    {
        MirType *baseType = base->getMirType();
        if (baseType->getKind() != MirTypeKind::Pointer)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref << "Can't create a memory operand if the base register doesn't have pointer type "
                    << type->getName();
            return nullptr;
        }
    }

    MirTypeTable *t = m_ctx->getTypeTable();
    return build<MirMemory>(type, base, build<MirInteger>(t->i64(), displ, nullptr), nullptr, 1, ref);
}

/**
 * Constructs a memory operand [base + index*scale + displ] using an existing MirInteger displacement operand.
 */
MirMemory *MirOperandBuilder::buildMem(
        MirType *type, MirRegister *base, MirInteger *displ, MirRegister *index, uint8_t scale, SourceReference *ref)
{
    if (base)
    {
        MirType *baseType = base->getMirType();
        if (baseType->getKind() != MirTypeKind::Pointer)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref << "Can't create a memory operand if the base register doesn't have pointer type "
                    << type->getName();
            return nullptr;
        }
    }
    if (index)
    {
        MirType *indexType = index->getMirType();
        if (indexType->getKind() != MirTypeKind::Integer && indexType->getKind() != MirTypeKind::Pointer)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref
                    << "Can't create a memory operand if the index register doesn't have integer or pointer type "
                    << type->getName();
            return nullptr;
        }
    }

    return build<MirMemory>(type, base, displ, index, scale, ref);
}

/**
 * Constructs a memory operand [base + index*scale + displ] converting an immediate FlexInt offset to a 64-bit integer
 * operand.
 */
MirMemory *MirOperandBuilder::buildMem(
        MirType *type, MirRegister *base, const FlexInt &displ, MirRegister *index, uint8_t scale, SourceReference *ref)
{
    if (base)
    {
        MirType *baseType = base->getMirType();
        if (baseType->getKind() != MirTypeKind::Pointer)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref << "Can't create a memory operand if the base register doesn't have pointer type "
                    << type->getName();
            return nullptr;
        }
    }
    if (index)
    {
        MirType *indexType = index->getMirType();
        if (indexType->getKind() != MirTypeKind::Integer && indexType->getKind() != MirTypeKind::Pointer)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "MirOperandBuilder")
                    << ref
                    << "Can't create a memory operand if the index register doesn't have integer or pointer type "
                    << type->getName();
            return nullptr;
        }
    }

    MirTypeTable *t = m_ctx->getTypeTable();
    return build<MirMemory>(type, base, build<MirInteger>(t->i64(), displ, nullptr), index, scale, ref);
}

/**
 * Allocates and registers a new virtual SSA register operand with a unique MIR ID.
 */
MirRegister *
MirOperandBuilder::buildVReg(MirType *type, std::string_view name, SourceReference *ref, MirRegisterClass *_class)
{
    std::pmr::string pmrName(name, m_allocator);
    MirRegister *reg = build<MirRegister>(type, true, m_ctx->createId(), ref, _class, std::move(pmrName));
    m_ctx->appendRegister(reg);

    return reg;
}

/**
 * Allocates a physical hardware register operand bound to a physical register ID and register class.
 */
MirRegister *MirOperandBuilder::buildPhysReg(
        MirType *type, MirPhysicalRegId physId, std::string_view name, MirRegisterClass *_class, SourceReference *ref)
{
    std::pmr::string pmrName(name, m_allocator);
    return build<MirRegister>(type, false, physId, ref, _class, std::move(pmrName));
}

/**
 * Builds a symbolic reference operand pointing to a basic block label (void pointer type).
 */
MirReference *MirOperandBuilder::buildRef(MirBlock *block, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(t->_void());

    return build<MirReference>(ptr, MirReferenceType::Block, block->getId(), 0, ref);
}

/**
 * Builds a symbolic reference operand pointing to a function (function pointer type).
 */
MirReference *MirOperandBuilder::buildRef(MirFunction *func, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(func->getType());

    return build<MirReference>(ptr, MirReferenceType::Function, func->getId(), 0, ref);
}

/**
 * Builds a symbolic reference operand pointing to a global variable at a specified byte displacement.
 */
MirReference *MirOperandBuilder::buildRef(MirGlobalVar *var, size_t offset, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(var->getType());

    return build<MirReference>(ptr, MirReferenceType::GlobalVar, var->getId(), offset, ref);
}

/**
 * Builds a symbolic reference operand pointing to a stack frame object slot.
 */
MirReference *MirOperandBuilder::buildRef(StackFrameObject *obj, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *fieldPtrType = t->getPtr(obj->m_type);

    return build<MirReference>(fieldPtrType, MirReferenceType::StackFrameObject, obj->m_id, 0, ref);
}

/**
 * Builds a named runtime library symbol operand.
 */
MirRuntimeSymbol *MirOperandBuilder::buildRtSymbol(std::pmr::string symbolName, SourceReference *ref)
{
    MirTypeTable *t = m_ctx->getTypeTable();
    MirType *ptr = t->getPtr(t->_void());

    return build<MirRuntimeSymbol>(ptr, std::move(symbolName), ref);
}
