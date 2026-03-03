#include "AstLowererVisitor/ImmediateLowerer.h"

bool ImmediateLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    ImmediateOperand *imm = dynamic_cast<ImmediateOperand *>(node);
    Type *type = nullptr;

    if (imm->getAnnotation<DataTypeAnnotation>())
    {
        type = imm->getAnnotation<DataTypeAnnotation>()->getDataType();
    }
    else
    {
        type = imm->getAnnotation<TypeCastAnnotation>()->getCastedDataType();
    }

    switch (imm->getImmediateType())
    {
        case ImmediateType::Integer:
        {
            IntegerImmediate *integer = dynamic_cast<IntegerImmediate *>(imm);

            if (type->getUnderlyingTypeSize() <= UnderlyingTypeSize::_64bits)
            {
                int64_t value = mp_get_i64(integer->getInteger());
                ctx->pushOperand(MirInteger{ .m_value = value });
            }
            else
            {
                // BigInt.
                std::string encoded = integer->getAsBin();
                MirGlobalDataEntry *entry =
                        ctx->getGlobalDataEmitter()->createGlobalData(encoded.data(), encoded.size());

                ctx->pushOperand(MirReference{ .m_refId = entry->m_entryId });
            }
            break;
        }
        case ImmediateType::FloatingPoint:
        {
            FloatImmediate *floatImm = dynamic_cast<FloatImmediate *>(imm);
            ctx->pushOperand(MirDouble{ .m_value = floatImm->getFloatingValue() });

            break;
        }
        case ImmediateType::String:
        {
            StringImmediate *strImm = dynamic_cast<StringImmediate *>(imm);
            MirGlobalDataEntry *entry = ctx->getGlobalDataEmitter()->createGlobalString(strImm->getStr());

            ctx->pushOperand(MirReference{ .m_refId = entry->m_entryId });
            break;
        }

        default:
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Can't lower immediate because its type is not valid",
                                                 "ImmediateLowerer");
            return false;
        }
    }

    return true;
}
