#include "AstLowererVisitor/MemoryLowerer.h"

bool MemoryLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    MemoryOperandAstNode *mem = dynamic_cast<MemoryOperandAstNode *>(node);

    switch (mem->getMemoryOperandType())
    {
        case MemoryOperandType::BaseDisplacement:
            return lowerBaseDisplacement(dynamic_cast<BaseDisplacementMemory *>(mem), ctx);
        case MemoryOperandType::BaseIndexScaleDisplacement:
            return lowerBaseIndexScaleDisplacement(dynamic_cast<BaseIndexScaleDisplacementMemory *>(mem), ctx);
        case MemoryOperandType::IndexScale:
            return lowerIndexScale(dynamic_cast<IndexScaleMemory *>(mem), ctx);
        case MemoryOperandType::Direct:
            return lowerDirect(dynamic_cast<DirectMemory *>(mem), ctx);

        default:
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Unsupported memory operand type in MemoryLowerer",
                                                 "MemoryLowerer");
            return false;
        }
    }

    return true;
}

bool MemoryLowerer::lowerBaseDisplacement(BaseDisplacementMemory *node, LoweringContext *ctx)
{
    Variable *base = node->getBase();
    IntegerImmediate *displ = node->getDisplacement();
    Type *dataType = node->getAnnotation<DataTypeAnnotation>()->getDataType();

    VariableLowerer varLowerer;
    if (!varLowerer.lower(base, ctx))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "Failed to lower base variable in BaseDisplacementMemory",
                                             "MemoryLowerer");
        return false;
    }

    int64_t value = displ ? mp_get_i64(displ->getInteger()) : 0;
    MirMemory memOperand{ .m_baseRegId = ctx->popOperand().getRegister()->m_id,
                          .m_indexRegId = 0,
                          .m_scale = 0,
                          .m_offset = value,
                          .m_size = static_cast<size_t>(dataType->getUnderlyingTypeSize()) / 8 };
    ctx->pushOperand(MirOperand{ memOperand });

    return true;
}

bool MemoryLowerer::lowerBaseIndexScaleDisplacement(BaseIndexScaleDisplacementMemory *node, LoweringContext *ctx)
{
    Variable *base = node->getBase(), *index = node->getIndex();
    IntegerImmediate *displ = node->getDisplacement(), *scale = node->getScalingFactor();
    Type *dataType = node->getAnnotation<DataTypeAnnotation>()->getDataType();

    VariableLowerer varLowerer;
    if (!varLowerer.lower(base, ctx))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "Failed to lower base variable in BaseIndexScaleDisplacementMemory",
                                             "MemoryLowerer");
        return false;
    }
    else if (!varLowerer.lower(index, ctx))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "Failed to lower index variable in BaseIndexScaleDisplacementMemory",
                                             "MemoryLowerer");
        return false;
    }

    int64_t displValue = mp_get_i64(displ->getInteger());
    int8_t scaleValue = static_cast<int8_t>(mp_get_i64(scale->getInteger()));
    size_t indexRegId = ctx->popOperand().getRegister()->m_id, baseRegId = ctx->popOperand().getRegister()->m_id;

    MirMemory memOperand{ .m_baseRegId = baseRegId,
                          .m_indexRegId = indexRegId,
                          .m_scale = scaleValue,
                          .m_offset = displValue,
                          .m_size = static_cast<size_t>(dataType->getUnderlyingTypeSize()) / 8 };
    ctx->pushOperand(MirOperand{ memOperand });

    return true;
}

bool MemoryLowerer::lowerIndexScale(IndexScaleMemory *node, LoweringContext *ctx)
{
    Variable *index = node->getIndex();
    IntegerImmediate *scale = node->getScalingFactor();
    Type *dataType = node->getAnnotation<DataTypeAnnotation>()->getDataType();

    VariableLowerer varLowerer;
    if (!varLowerer.lower(index, ctx))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "Failed to lower index variable in IndexScaleMemory",
                                             "MemoryLowerer");
        return false;
    }

    int8_t value = static_cast<int8_t>(mp_get_i64(scale->getInteger()));
    MirMemory memOperand{ .m_baseRegId = 0,
                          .m_indexRegId = ctx->popOperand().getRegister()->m_id,
                          .m_scale = value,
                          .m_offset = 0,
                          .m_size = static_cast<size_t>(dataType->getUnderlyingTypeSize()) / 8 };
    ctx->pushOperand(MirOperand{ memOperand });

    return true;
}

bool MemoryLowerer::lowerDirect(DirectMemory *node, LoweringContext *ctx)
{
    IntegerImmediate *displ = node->getAddress();
    int64_t value = mp_get_i64(displ->getInteger());
    Type *dataType = node->getAnnotation<DataTypeAnnotation>()->getDataType();

    MirMemory memOperand{ .m_baseRegId = 0,
                          .m_indexRegId = 0,
                          .m_scale = 0,
                          .m_offset = value,
                          .m_size = static_cast<size_t>(dataType->getUnderlyingTypeSize()) / 8 };
    ctx->pushOperand(MirOperand{ memOperand });

    return true;
}
