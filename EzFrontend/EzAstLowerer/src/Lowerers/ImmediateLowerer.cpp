#include "Lowerers/ImmediateLowerer.h"

bool ImmediateLowerer::lower(AstNode *node, AstLoweringContext *ctx)
{
    ImmediateOperand *imm = dynamic_cast<ImmediateOperand *>(node);
    Type *type = imm->getAnnotation<DataTypeAnnotation>()->getDataType();
    MirType *mirType = ctx->createMirTypeFromSemanticType(type);

    switch (imm->getImmediateType())
    {
        case ImmediateType::Integer:
        {
            IntegerImmediate *integer = dynamic_cast<IntegerImmediate *>(imm);

            if (type->getUnderlyingTypeSize() <= UnderlyingTypeSize::_64bits)
            {
                int64_t value = mp_get_i64(integer->getInteger());
                ctx->pushOperand(MirInteger{ .m_value = value,
                                             .m_sizeInBytes = static_cast<size_t>(type->getUnderlyingTypeSize()) / 8 },
                                 mirType);
            }
            else
            {
                // BigInt.
                std::string encoded = integer->getAsBin();
                MirGlobalDataEntry *entry = ctx->getEmitterContext()->createGlobalData(encoded.data(), encoded.size());
                MirReference ref = MirReference{ .m_type = MirReferenceType::DataEntry, .m_refId = entry->m_entryId };
                ctx->pushOperand(ref, mirType);
            }
            break;
        }
        case ImmediateType::FloatingPoint:
        {
            FloatImmediate *floatImm = dynamic_cast<FloatImmediate *>(imm);
            ctx->pushOperand(MirDouble{ .m_value = floatImm->getFloatingValue() }, mirType);

            break;
        }
        case ImmediateType::String:
        {
            StringImmediate *strImm = dynamic_cast<StringImmediate *>(imm);
            MirGlobalDataEntry *entry = ctx->getEmitterContext()->createGlobalString(strImm->getStr());

            ctx->pushOperand(MirReference{ .m_type = MirReferenceType::DataEntry, .m_refId = entry->m_entryId },
                             mirType);
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
